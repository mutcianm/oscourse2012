#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <queue>
#include <poll.h>

#define BUF_SIZE 32*1024*1024

using namespace std;

void readTZ(int sockfd) {
    bool done = false;
    char buffer[256];
    while (!done) {
	int cnt = read(0, buffer, 256);
	write(sockfd, buffer, cnt);
//	for (int i = 0; i < cnt; i++) {
	   // if (buffer[i] == '\0') {
//		done = true;
//	    }
//	}
        if (cnt <= 0) {
	    done = true;
	}
    }
}

int main(int argc, char *argv[]) {
    	int sockfd, portno, n;
    	struct sockaddr_in serv_addr;
    	struct hostent *server;

    	char buffer[256];
    	if (argc < 3) {
		fprintf(stderr,"%s hostname port\n", argv[0]);
		exit(0);
    	}
    	portno = atoi(argv[2]);
    	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("Error opening socket");
		exit(1);
    	}
    	server = gethostbyname(argv[1]);
	if (server == NULL) {
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
    	}
    	bzero((char *) &serv_addr, sizeof(serv_addr));
    	serv_addr.sin_family = AF_INET;
    	bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
    	serv_addr.sin_port = htons(portno);
    	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0)
		perror("connect error");
     	printf("Connected!\n");
     	char buf[18];
	//subscribe XXXXXX unsubscribe XXXXXX emit XXXXXX list
	
	vector<string> signals;
	queue<string> messageQueue;
	pollfd pollfds[2];
	pollfds[0].fd = sockfd;
	pollfds[0].events = POLLIN | POLLOUT;
	pollfds[0].revents = 0;
	pollfds[1].fd = 1;
	pollfds[1].events = POLLIN | POLLOUT;
	pollfds[1].revents = 0;
	while (true) {
		//printf("Pass\n");
		int ret = poll(pollfds,2,1);
		if (ret <= 0) {
			continue;
		}
		if (pollfds[0].revents & POLLIN) {
			//some data from server
			char buf2[100];
			int cnt = read(sockfd, &buf2, 100);
			printf(buf2);
			memset(&buf2,0,100);
		}
		if (pollfds[0].revents & POLLOUT) {
			//server rdy to receive
			if (messageQueue.size() > 0) {
				string mess = messageQueue.front();
				messageQueue.pop();
				printf("Sending message %s\n", mess.c_str());
				int cnt = write(sockfd, mess.c_str(), mess.length());
				if (cnt < mess.length()) {
					perror("couldn't send message");
				}
			}
		}
		if (pollfds[1].revents & POLLIN) {
			//some data from stdin
			char buf[100];
			int cnt = read(pollfds[1].fd,&buf, 100);	
		//	printf("%s subscribe %d\n",buf, strncmp(buf, "subscribe", 9));
		//	printf("%s unsubscribe %d\n", buf, strncmp(buf, "unsubscribe", 11));
		//	printf("%s emit %d\n", buf, strncmp(buf, "emit", 4));
		//	printf("%s list %d\n", buf, strncmp(buf, "list", 4));

			if (strncmp(buf,"subscribe",9) == 0) {
				//subscribe message
				string subname = string(&buf[10],6);
				if (strlen(buf) < 16) {
					printf("Error\n");
					continue;
				}
				bool subscribed = false;
				for (int i = 0; i < signals.size(); i++) {
					if (signals[i] == subname) {
						subscribed = true;
						break;
					}
				}
				if (!subscribed) {
					printf("Subscribing to signal: %s\n", subname.c_str());
					string mess = "sub " + subname;
					messageQueue.push(mess);
					mess = string(subname);
					signals.push_back(mess);
				} else {
					printf("Already subscribed to signal: %s\n", subname.c_str());
				}
			} else if (strncmp(buf,"unsubscribe", 11) == 0) {
				//unsubscribe	
				if (signals.size() == 0) {
					printf("Can't unsubscribe, no subscribed signals!\n");
					continue;
				}
				if (strlen(buf) < 18) {
					printf("Error\n");
					continue;
				}
			//	printf("unsub - %s\n", buf);
				string subname = string(&buf[12],6);
				bool subscribed = false;
				int k = -1;
				//printf("%d registered signals\n", signals.size());o
				if (signals.size() > 0) {
					for (int i = 0; i < signals.size(); i++) {
						if (signals[i] == subname) {
							subscribed = true;
							k = i;
							break;	
						}	
					}
				}
				if (subscribed) {
					printf("Unsubscribing from signal: %s\n", subname.c_str());
					string mess = "uns " + subname;
					messageQueue.push(mess);
					signals.erase(signals.begin() + k);
				} else {
					printf("Can't unsubscribe - not subscribed to signal %s\n");
				}
			} else if (strncmp(buf,"emit",4) == 0) {
				//emit
				if (strlen(buf) < 11) {
					printf("Error\n");
				}
				string subname = string(&buf[5], 6);
				printf("Emitting signal %s\n", subname.c_str());
				string mess = "emt " + subname;
				messageQueue.push(mess);
			} else if (strncmp(buf,"list", 4) == 0) {
				//list
				printf("Ordered a list\n");
				string mess = "lst";
				messageQueue.push(mess);
			} else if (strncmp(buf, "exit", 4) == 0) {
				exit(0);
			} else {
				printf("unrecognised command, use \"subscribe XXXXXX\", \"unsubscribe XXXXXX\", \"emit XXXXXX\" or \"list\"\n");
			}
			memset(&buf,0,100);
		}		

	}
     	return 0;
}

