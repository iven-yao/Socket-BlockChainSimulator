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
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

#define MYPORT_UDP "24200"	  // the port use UDP
#define MYPORT_TCP_A "25200"  // the port use TCP for clientA
#define MYPORT_TCP_B "26200"  // the port use TCP for clientB
#define SERVERPORT_A "21200"  // port for serverA
#define SERVERPORT_B "22200"  // port for serverB
#define SERVERPORT_C "23200"  // port for serverC
#define HOST "127.0.0.1" 	  // local address
#define BACKLOG 10
#define MAXDATALEN 100
#define MAXBUFLEN 1024
#define FILENAME "alichain.txt"

typedef struct{
	int serial;
	char payer[MAXDATALEN];
	char payee[MAXDATALEN];
	int amount;
}list_item;

// from Beej's
void sigchld_handler(int s) {
	int saved_errno = errno;

	while(waitpid(-1,NULL, WNOHANG) > 0);

	errno = saved_errno;
}

// setting part from Beej's
int setupTCP(char* port) {
	int rv;
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	struct sigaction sa;
	int yes = 1;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
   
	if((rv = getaddrinfo(HOST, port, &hints, &servinfo)) !=0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
   
	for(p = servinfo; p!=NULL; p= p->ai_next) {
		if((sockfd = socket(p->ai_family, 
		  p->ai_socktype, p->ai_protocol)) == -1) {
				          
			perror("server: socket");
			continue;                    
		}

		if(setsockopt(sockfd, SOL_SOCKET, 
		 SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);   
		}

		if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}
   
	freeaddrinfo(servinfo);

	if(p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if(listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler;
	sa.sa_flags = SA_RESTART;
	if(sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	return sockfd;
}

char* serverName(char* port) {
	if(strcmp(port, "21200") == 0) {
		return "A";
	} else if(strcmp(port, "22200") == 0) {
		return "B";
	} else if(strcmp(port, "23200") == 0) {
		return "C";
	}
	
	return "0";
}

char* clientName(char* port) {
	if(strcmp(port, "25200") == 0) {
		return "A";
	} else if(strcmp(port, "26200") == 0) {
		return "B";
	}
	
	return "0";
}

int transfer(char* port, int serial, char* payer, char* payee, int amount){
	// setting part from Beej's
	int sockfd;
	int rv;
	int numbytes; 
	struct addrinfo hints, *servinfo, *p;
	char buf[MAXBUFLEN];
	char msg[MAXBUFLEN];
	socklen_t addr_len;
	struct sockaddr_storage their_addr;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	
	if(( rv = getaddrinfo(HOST, port, &hints, &servinfo)) != 0 ) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	}
	
	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
		p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
	}
	
	sprintf(msg, "transfer::%d::%s::%s::%d",serial, payer, payee, amount);
	
	if ((numbytes = sendto(sockfd, msg , strlen(msg), 0,
		p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

	freeaddrinfo(servinfo);

	printf("The main server sent a request to server %s\n", serverName(port));
   
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
	(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}   
   
	buf[numbytes] = '\0';
	printf("The main server received feedback from Server %s using UDP over port %s.\n", serverName(port), port);
   
	close(sockfd);
	
	return atoi(buf);
}

//balance(1), exist(2), serial(3)
int query(char* port, char* username, int flag) {
	// setting part from Beej's
	int sockfd;
	int rv;
	int numbytes; 
	struct addrinfo hints, *servinfo, *p;
	char buf[MAXBUFLEN];
	char msg[MAXBUFLEN];
	socklen_t addr_len;
	struct sockaddr_storage their_addr;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	
	if(( rv = getaddrinfo(HOST, port, &hints, &servinfo)) != 0 ) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	}
	
	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
		p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
	}
	
	if(flag == 1) {
		sprintf(msg, "balance::%s", username);
	} else if(flag == 2) {
		sprintf(msg, "exist::%s", username);
	} else if(flag == 3) {
		sprintf(msg, "serial");
	}
	
	if ((numbytes = sendto(sockfd, msg , strlen(msg), 0,
		p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

	freeaddrinfo(servinfo);

	printf("The main server sent a request to server %s\n", serverName(port));
   
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
	(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}   
   
	buf[numbytes] = '\0';
	printf("The main server received transactions from Server %s using UDP over port %s.\n", serverName(port), port);
   
	close(sockfd);
	
	return atoi(buf);
}

int max(int i1, int i2) {
	if(i1> i2) return i1;
	return i2;
}

int queryAllServer(char* username, int flag){
	if(flag == 1) {
		return query(SERVERPORT_A, username,1)+
				query(SERVERPORT_B, username,1)+
				query(SERVERPORT_C, username,1);
	} else if (flag == 2) {
		return query(SERVERPORT_A, username, 2)||
				query(SERVERPORT_B, username, 2)||
				query(SERVERPORT_C, username, 2);
	} else if (flag == 3) {
		int a = query(SERVERPORT_A, "",3);
		int b = query(SERVERPORT_B, "",3);
		int c = query(SERVERPORT_C, "",3);
		return max(a,max(b,c))+1;
	}
	
	return 0;
}

char* getServerPort(int r) {
	if(r == 0) return SERVERPORT_A;
	else if(r == 1) return SERVERPORT_B;
	else return SERVERPORT_C;
}

void getTXList(char* port, char* list) {
	// setting part from Beej's
	int sockfd;
	int rv;
	int numbytes; 
	struct addrinfo hints, *servinfo, *p;
	char buf[MAXBUFLEN] ="";
	char msg[MAXBUFLEN];
	socklen_t addr_len;
	struct sockaddr_storage their_addr;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	
	if(( rv = getaddrinfo(HOST, port, &hints, &servinfo)) != 0 ) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	}
	
	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
		p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
	}
	
	sprintf(msg, "TXLIST");
	
	if ((numbytes = sendto(sockfd, msg , strlen(msg), 0,
		p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

	freeaddrinfo(servinfo);

	printf("The main server sent a request to server %s\n", serverName(port));
   
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
	(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}   
   
	buf[numbytes] = '\0';
	printf("The main server received transactions from Server %s using UDP over port %s.\n", serverName(port), port);
   
	close(sockfd);
	strcat(list, buf);
	
}

void getAllTXList(char* list) {
	getTXList(SERVERPORT_A, list);
	getTXList(SERVERPORT_B, list);
	getTXList(SERVERPORT_C, list);
}

int compare(const void * a, const void * b) {
	list_item *list_a = (list_item*) a;
	list_item *list_b = (list_item*) b;
	return list_a->serial-list_b->serial;
}

int writeToFile(char* list) {
	int list_size = queryAllServer("",3)-1; // get max serial #
	list_item l[list_size];
	int i = 0;
	char* token = strtok(list,"::");
	while (token != NULL){
		sscanf(token, "%d %s %s %d", &l[i].serial, l[i].payer, l[i].payee, &l[i].amount);
		i++;
		token = strtok(NULL,"::");
	}
	
	qsort(l, list_size, sizeof(list_item), compare);
	
	FILE* ptr = fopen(FILENAME, "w");
	if (ptr == NULL) {
		printf("no such file.");
		return 0;
	}
	
	int n = 0;
	for(n = 0; n < list_size; n++) {
		fprintf(ptr, "%d %s %s %d\n",l[n].serial, l[n].payer, l[n].payee, l[n].amount);
	}
	
	fclose(ptr);
	
	return 1;

}

void TCPConnection(int child_sockfd, char* port, struct sockaddr_storage their_addr) {
	
	char s[INET6_ADDRSTRLEN];
	int numbytes; 
	// temp saving place for receiving from client through TCP
	char message_buf[MAXBUFLEN];
	// temp saving place for sending results to client through TCP  
	char result_buf[MAXBUFLEN];

	int r = rand()%3;

	if(!fork()) {

		// receive arguments as single string
		if((numbytes = recv(child_sockfd, message_buf, 
		sizeof message_buf, 0)) == -1) {
		perror("recv");
		exit(1);
		}

		message_buf[numbytes] = '\0';
		fflush(stdout);
		


		char username[MAXBUFLEN];
		char receiver[MAXBUFLEN];
		int amount = 0;

		char *token = strtok(message_buf, "::");
		strcpy(username, token);
		token = strtok(NULL, "::");
		if(token) {
		strcpy(receiver, token);
		token = strtok(NULL, "::");
		amount = atoi(token);
		}

		if(amount > 0) {
			printf("The main server received from \"%s\" to transfer %d coins to \"%s\" using TCP over port %s.\n",username, amount, receiver, port);
			int exist_payer = queryAllServer(username, 2);
			int exist_payee = queryAllServer(receiver, 2);
			
			if(exist_payer && exist_payee) {
			// both in the network
				int payer_balance = queryAllServer(username,1);
			
				if(1000+payer_balance < amount) {
				// not enough coins for payer
					sprintf(result_buf, "0::%d", 1000+payer_balance);
					if(send(child_sockfd, result_buf, sizeof result_buf, 0) == -1) {
						perror("send");
					}
				} else {
				// do transfer
				// get serial #
					int serial = queryAllServer("",3);
					char* p = getServerPort(r);
					if(transfer(p, serial, username, receiver, amount)){
					// transfer write successfully
					// return payer's balance
						int balance = queryAllServer(username, 1);
						sprintf(result_buf, "1::%d", 1000+balance);
						if(send(child_sockfd, result_buf, sizeof result_buf, 0) == -1) {
							perror("send");
						}
					}
				}
				
				printf("The main server sent the result of the transaction to client %s.\n", clientName(port));
			
			} else if(!exist_payer && !exist_payee) {
			// both not in the network
				sprintf(result_buf, "3");
				if(send(child_sockfd, result_buf, sizeof result_buf, 0) == -1) {
					perror("send");
				}
			
			} else if(!exist_payer) {
			// payer not in the network
				sprintf(result_buf, "2::%s", username);
				if(send(child_sockfd, result_buf, sizeof result_buf, 0) == -1) {
					perror("send");
				}
			
			} else if(!exist_payee) {
			// payee not in the network
				sprintf(result_buf, "2::%s", receiver);
				if(send(child_sockfd, result_buf, sizeof result_buf, 0) == -1) {
					perror("send");
				}
			
			}
			

		} else {
			if(strcmp(username, "TXLIST") == 0 ) {
				char txList[MAXBUFLEN] = "";
				getAllTXList(txList);
				writeToFile(txList);
			} else {
	   		printf("The main server received input=\"%s\" from the client using TCP over port %s.\n", username, port);
			
				int exist = queryAllServer(username,2);
				if(!exist) {
					sprintf(result_buf, "0");
					if(send(child_sockfd, result_buf, sizeof result_buf, 0) == -1) {
						perror("send");
					}
				} else {
					int balance = queryAllServer(username,1);

					sprintf(result_buf, "1::%d", 1000+balance);
					if(send(child_sockfd, result_buf, sizeof result_buf, 0) == -1) {
						perror("send");
					}
				}
				printf("The main server sent the current balance to client %s.\n", clientName(port));
			}
		}

		close(child_sockfd);

		exit(0);
	}
}

int main(void) {

	// set rand seed
	srand(time(NULL));

	int sockfd_a, new_fd_a, sockfd_b, new_fd_b;
	int udp_sock;
	// connector's address information
	struct sockaddr_storage their_addr_a, their_addr_b;
	// used for accept() 
	socklen_t sin_size; 

	// set TCP sockets and make them nonblock
	sockfd_a = setupTCP(MYPORT_TCP_A);
	sockfd_b = setupTCP(MYPORT_TCP_B);
	fcntl(sockfd_a, F_SETFL, O_NONBLOCK);
	fcntl(sockfd_b, F_SETFL, O_NONBLOCK);

	printf("The main server is up and running.\n");
   
	while(1) {
		sin_size = sizeof their_addr_a;
	
		new_fd_a = accept(sockfd_a, (struct sockaddr *)&their_addr_a,
		&sin_size);

		if(new_fd_a != -1) {
			// a connecting...
			TCPConnection(new_fd_a, MYPORT_TCP_A, their_addr_a);
			close(new_fd_a);
		}
		
		new_fd_b = accept(sockfd_b, (struct sockaddr *)&their_addr_b,
		&sin_size);

		if(new_fd_b != -1) {
			// b connecting...
			TCPConnection(new_fd_b, MYPORT_TCP_B, their_addr_b);
			close(new_fd_b);
		}
	}
	return 0;   
}
