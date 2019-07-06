#include "common.h"
#define LISTENQ 100
#include<signal.h>

int Socket(char *host, char *port)
{
	struct addrinfo hint, *listp, *p;
	memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	if(host==NULL)
		hint.ai_flags = AI_PASSIVE;
	int err = getaddrinfo(host, port, &hint, &listp);
	if(err!=0)
	{
		printf("getaddrinfor:%s\n", gai_strerror(err));
	}
	int sockfd;
	for(p=listp;p!=NULL;p=p->ai_next)
	{
		if((sockfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))<0)
		{
			perror("socket");
			exit(-1);
		}
		if(host==NULL)
		{
			if(bind(sockfd, p->ai_addr, p->ai_addrlen)==0)
				break;
		}
		else{
			if(connect(sockfd, p->ai_addr, p->ai_addrlen)==0)
				break;
		}
		close(sockfd);
	}
	if(p==NULL)
	{
		perror("Socket");
		exit(-1);
	}
	return sockfd;
}
void client(char *host, char *port)
{
	int sockfd = Socket(host, port);
	printf("socket : %d\n", sockfd);
	close(sockfd);
}

void server(char *port)
{
	int listenfd = Socket(NULL, port);
	if(listen(listenfd, LISTENQ)!=0)
	{
		perror("listen");
		exit(-1);
	}
	printf("listenfd:%d\n", listenfd);
	int acceptfd = accept(listenfd, static_cast<struct sockaddr*>(nullptr), static_cast<socklen_t *>(nullptr));
	if(acceptfd<0)
		perror("accept");
	printf("acceptfd:%d\n", acceptfd);	
	char buf[10000] = "abcd\n";
	for(int i=0;i<5;++i)
	{
		int writen  = write(acceptfd, buf, sizeof(buf));
		printf("write : %d\n", writen);
		sleep(3);
	}
}

void sig_pip(int)
{
	printf("capture SIGPIPE\n");
}
int main(int argc, char *argv[])
{
	if(signal(SIGPIPE, sig_pip)==SIG_ERR)
	{
		printf("can not catch sigpipe");
		exit(-1);
	}
	if(argc==4&&strcmp(argv[1],"-c")==0)
	{
		client(argv[2], argv[3]);
	}
	else if(argc==3&&strcmp(argv[1],"-s")==0)
	{
		server(argv[2]);
	}
	else {
		printf("error usage!\n");
	}
}
