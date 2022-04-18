/*
** ServerC
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVERNAME "ServerC"
#define MYPORT "23200"
#define MAXBUFLEN 1024
#define FILENAME "block3.txt"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int checkBalance(char* username) {
	FILE* ptr = fopen(FILENAME, "r");
	if (ptr == NULL) {
		printf("no such file.");
		return 0;
	}

	int serial;
	char payer[MAXBUFLEN];
	char payee[MAXBUFLEN];
	int amount;
	int diff = 0;

	while (fscanf(ptr, "%d %s %s %d",&serial, payer, payee, &amount) == 4){
		if(strcmp(payer, username) == 0) {
			diff -= amount;
		}
		
		if(strcmp(payee, username) == 0) {
			diff += amount;
		}
	}

	fclose(ptr);

	return diff;
}

int checkExist(char* username) {
	FILE* ptr = fopen(FILENAME, "r");
	if (ptr == NULL) {
		printf("no such file.");
		return 0;
	}

	char payer[MAXBUFLEN];
	char payee[MAXBUFLEN];

	while (fscanf(ptr, "%*d %s %s %*d",payer, payee) == 2){
		if(strcmp(payer, username) == 0) return 1;
		if(strcmp(payee, username) == 0) return 1;
	}
	
	fclose(ptr);

	return 0;
}

int getLargestSerial() {
	FILE* ptr = fopen(FILENAME, "r");
	if (ptr == NULL) {
		printf("no such file.");
		return 0;
	}

	int serial;
	int largest = 0;

	while (fscanf(ptr, "%d %*s %*s %*d",&serial) == 1){
		if(serial > largest) largest = serial;
	}
	
	fclose(ptr);

	return serial;
}

int writeToServer(char* serial, char* payer, char* payee, char* amount) {
	FILE* ptr = fopen(FILENAME, "a");
	if (ptr == NULL) {
		printf("no such file.");
		return 0;
	}
	
	fprintf(ptr, "%s %s %s %s\n", serial, payer, payee, amount);
	
	fclose(ptr);
	
	return 1;
}

void getTXList(char* buf) {
	strcpy(buf, "");

	FILE* ptr = fopen(FILENAME, "r");
	if (ptr == NULL) {
		printf("no such file.");
	}

	int serial;
	char payer[MAXBUFLEN];
	char payee[MAXBUFLEN];
	int amount;

	while (fscanf(ptr, "%d %s %s %d",&serial, payer, payee, &amount) == 4){
		sprintf(buf+strlen(buf),"%d %s %s %d::",
								serial, payer,payee,amount);
	}

	fclose(ptr);
}

int main(void)
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	printf("The %s is up and running using UDP on port %s.\n", SERVERNAME, MYPORT);

	addr_len = sizeof their_addr;
	while(1) {

		if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
		   (struct sockaddr *)&their_addr, &addr_len)) == -1) {
		   perror("recvfrom");
		   exit(1);
		}

		printf("The %s received a request from the Main Server.\n", SERVERNAME);
		buf[numbytes] = '\0';
		
		char* token = strtok(buf, "::");
		if(strcmp(token, "balance") == 0) {
		// balance_username
			token = strtok(NULL,"::");
			int balance = checkBalance(token);
			char payload[10];
			sprintf(payload, "%d", balance);
		  
			if ((numbytes = sendto(sockfd, payload , strlen(payload), 0,
				(struct sockaddr *)&their_addr, sizeof their_addr)) == -1) {
				perror("listener: sendto");
				exit(1);
			}
		
		} else if(strcmp(token, "exist") == 0) {
		// exist_username
			token = strtok(NULL,"::");
			int exist = checkExist(token);
			char payload[10];
			sprintf(payload, "%d", exist);
			if ((numbytes = sendto(sockfd, payload , strlen(payload), 0,
				(struct sockaddr *)&their_addr, sizeof their_addr)) == -1) {
				perror("listener: sendto");
				exit(1);
			}
		
		} else if(strcmp(token, "transfer") == 0) {
		// transfer_serial_payer_payee_amount
			char* serial = strtok(NULL,"::");
			char* payer = strtok(NULL,"::");
			char* payee = strtok(NULL,"::");
			char* amount = strtok(NULL,"::");
			
			int returnValue = writeToServer(serial, payer, payee, amount);
			char payload[10];
			sprintf(payload, "%d", returnValue);
			
			if ((numbytes = sendto(sockfd, payload , strlen(payload), 0,
				(struct sockaddr *)&their_addr, sizeof their_addr)) == -1) {
				perror("listener: sendto");
				exit(1);
			}
		
		} else if(strcmp(token, "serial") == 0) {
		// serial
			int serial = getLargestSerial();
			char payload[10];
			sprintf(payload, "%d", serial);
		  
			if ((numbytes = sendto(sockfd, payload , strlen(payload), 0,
				(struct sockaddr *)&their_addr, sizeof their_addr)) == -1) {
				perror("listener: sendto");
				exit(1);
			}
		
		} else if(strcmp(token, "TXLIST") == 0) {
			char payload[MAXBUFLEN]; 
			getTXList(payload);
			if ((numbytes = sendto(sockfd, payload , strlen(payload), 0,
				(struct sockaddr *)&their_addr, sizeof their_addr)) == -1) {
				perror("listener: sendto");
				exit(1);
			}
		}
		
		printf("The %s finished sending the response to the Main Server.\n", SERVERNAME);
	}
   
	close(sockfd);

	return 0;
}
