#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "25200" // the port clientA will be connecting to 
#define MAXDATASIZE 1024 // max number of bytes we can get at once 

#define HOST "127.0.0.1"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	char payload[MAXDATASIZE];
	char username[MAXDATASIZE];
	char payer[MAXDATASIZE];
	char payee[MAXDATASIZE];
	char amount[MAXDATASIZE];
	
	if (argc == 2) {
	   strcpy(payload,argv[1]);
	   strcpy(username,argv[1]);
	} else if (argc == 4) {
	   sprintf(payload, "%s::%s::%s", argv[1], argv[2], argv[3]);
	   strcpy(payer,argv[1]);
	   strcpy(payee,argv[2]);
	   strcpy(amount,argv[3]);
	} else {
	   fprintf(stderr,"usage: invalid arguments\n");
      exit(1);
	}
	
	printf("The client A is up and running.\n");

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(HOST, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure
	
	// send
	
	if(send(sockfd, &payload, sizeof(payload), 0) == -1 ) {
	   perror("send");
	}
	
	if(argc == 2) {
		if(strcmp(argv[1],"TXLIST") == 0) {
			printf("Client A sent a sorted list request to the main server.\n");
		} else {
			printf("\"%s\" sent a balance enquiry request to the main server.\n",payload);
			if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
				perror("recv");
				exit(1);
			}

			buf[numbytes] = '\0';
		}
	} else if(argc == 4) {
		printf("\"%s\" has requested to transfer %s coins to \"%s\".\n", payer, amount, payee);
	   
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
			perror("recv");
			exit(1);
		}

		buf[numbytes] = '\0';
	}
	
	

   if(argc == 2) {
      if(strcmp(argv[1],"TXLIST") == 0) {
      
      } else {
			char* token = strtok(buf, "::");
			if(atoi(token) == 1) {
				token = strtok(NULL,"::");
				printf("The current balance of \"%s\" is : %s alicoins.\n",
				username, token);
			} else {
				printf("Unable to proceed with the request as \"%s\" is not part of the network.\n", username);
			}			
      }
   } else if(argc == 4) {
      char* token = strtok(buf, "::");
      if(atoi(token) == 1) {
         token = strtok(NULL,"::");
         printf("\"%s\" has successfully transferred %s alicoins to \"%s\".\nThe current balance of \"%s\" is :%s alicoins.\n", payer, amount, payee, payer, token);
      } else if(atoi(token) == 0) {
         token = strtok(NULL,"::");
         printf("\"%s\" was unable to transfer %s alicoins to \"%s\" because of insufficient balance.\nThe current balance of \"%s\" is :%s alicoins.\n", payer, amount, payee, payer, token);
      } else if(atoi(token) == 2) {
      // one of client not in the network
         token = strtok(NULL,"::");
         printf("Unable to proceed with the transaction as \"%s\" is not part of the network.\n", token);
      } else if(atoi(token) == 3) {
      // both clients not in the network
         printf("Unable to proceed with the transaction as \"%s\" and \"%s\" are not part of the network.\n", payer, payee);
      }
      
   }

	close(sockfd);

	return 0;
}

