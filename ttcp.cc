#include "common.h"
#include<errno.h>
#define LISTENQ 10000
#include<time.h>

int send_n(int sockfd, void *buf, int n)
{
	int nleft=n;
	int nsend;
	char *ptr = static_cast<char*>(buf);
	while(nleft>0)
	{
		if((nsend = send(sockfd, ptr, nleft, 0))<=0)
		{
			if(nsend<0&&errno == EINTR)
				nsend = 0;
			else return(-1);
		}
		nleft -= nsend;
		ptr += nsend;
	}
	return n;
}
struct SessionMsg{
	int number;
	int length;
};

struct PayloadMsg{
	int length;
	char data[0];
};

void recive(char *port)
{
	struct addrinfo hint, *listp, *p;
	memset(&hint, 0, sizeof(hint));
	hint.ai_flags = AI_PASSIVE;
	hint.ai_socktype = SOCK_STREAM;
	
	int err = getaddrinfo(NULL,port,&hint, &listp);
	if(err!=0)
	{
		printf("recive getaddrinfo:%s\n", gai_strerror(err));
		exit(-1);
	}

	int listenfd;
	for(p=listp;p!=NULL;p=p->ai_next)
	{
		if((listenfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))<0)
			continue;
		if(bind(listenfd,p->ai_addr, p->ai_addrlen)==0)
			break;
		close(listenfd);
	}
	freeaddrinfo(listp);
	if(p==NULL)
	{
		std::cout<<"open listen fd error"<<std::endl;
		exit(-1);
	}
	if(listen(listenfd, LISTENQ)!=0)
	{
		close(listenfd);
		perror("listen");
		exit(-1);
	}
	
	struct sockaddr_storage clientaddr;
	socklen_t clientlen = sizeof(clientaddr);
	int sockfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);

	struct SessionMsg ssMsg;
	if(recv(sockfd, &ssMsg, sizeof(ssMsg), MSG_WAITALL)!=sizeof(ssMsg))
	{
		perror("recv sessionmsg");
		exit(-1);
	}
	int number = ntohl(ssMsg.number);
	int length = ntohl(ssMsg.length);

	char hostname[50], portname[50];
	err = getnameinfo((struct sockaddr*)&clientaddr, clientlen, hostname, sizeof(hostname), portname, sizeof(portname), 0);
	if(err!=0)
	{
		printf("getnameinfo:%s", gai_strerror(err));
	}
	printf("accept %s:%s number = %d. length = %d\n", hostname, portname, number, length);
	int total_len = sizeof(int) + length;
	struct PayloadMsg* payload = static_cast<struct PayloadMsg*>(malloc(total_len));
	int nrecv, nsend;

	time_t start = time(NULL);
	for(int i=0;i<number;++i)
	{
		nrecv = recv(sockfd, payload, total_len, MSG_WAITALL);
		if(nrecv!=total_len)
		{
			perror("recv");
			break;
		}
		else {
			printf("recv %d\n", total_len);
		}
		int ack = length;
		ack = htonl(ack);
		nsend = send_n(sockfd, &ack, sizeof(ack));
		if(nsend!=sizeof(ack))
		{
			perror("send ack");
			break;
		}else{
			printf("send ack\n");
		}
	}
	double elapsed = time(NULL) - start;
	double total_mb = 1.0*length*number /1024/1024;
	printf("%.3f seconds\n%.3f MiB/s\n", elapsed, total_mb/elapsed);
	close(sockfd);
	free(payload);

}
void transmit(const char* host, char *port, std::string num, std::string len)
{
	struct addrinfo hint, *listp, *p;
	memset(&hint, 0, sizeof(hint));

	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	
	int err = getaddrinfo(host, port, &hint, &listp);
	if(err!=0)
	{
		printf("transmit:%s",gai_strerror(err));
		exit(-1);
	}

	int sockfd;
	for(p = listp;p!=NULL; p = p->ai_next)
	{
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(sockfd<0)
			continue;
		if(connect(sockfd, p->ai_addr, p->ai_addrlen)==0)
			break;
		close(sockfd);	
	}
	freeaddrinfo(listp);

	struct SessionMsg ssMsg = {0, 0};
	int number = std::stoi(num);
	int length = std::stoi(len);
	ssMsg.number = htonl(number);
	ssMsg.length = htonl(length);
	if(send_n(sockfd, &ssMsg, sizeof(ssMsg))!=sizeof(ssMsg))
	{
		perror("send SessionMessage");
	}
	
	const int total_len = sizeof(int) + length;
	struct PayloadMsg* payload = static_cast<struct PayloadMsg*>(malloc(total_len));
    if(payload==NULL)
	{
		perror("malloc");
		exit(-1);
	}
	payload->length = length;
	for(int i=0;i<length;++i)
	{
		payload->data[i] = "0123456789ABCDEF"[i % 16];
	}	
	//std::unique_ptr<struct PayloadMsg, decltype(free)> freeIt(payload, free);
	
	int nsend;
	for(int i=0;i<number;++i)
	{
		if((nsend = send_n(sockfd, payload, total_len))!=total_len)
		{
			std::cout<<"send:"<<nsend<<" != "<<total_len<<std::endl;
			break;
		}
		else{
			printf("send %d\n", nsend);
		}
		int ack;
		int nr = recv(sockfd,&ack,sizeof(ack), MSG_WAITALL);
		ack = ntohl(ack);
		if(ack!=length)
		{
			std::cout<<"ack:"<<ack<<std::endl;
			break;
		}else {
			printf("recv ack\n");
		}
	}
	close(sockfd);
	free(payload);

}
int main(int argc, char *argv[])
{
	if(argc==3&&strcmp(argv[1], "recv")==0)
	{
		recive(argv[2]);
	}
	else if(argc==6&&strcmp(argv[1], "trans")==0)
	{
		transmit(argv[2],argv[3], argv[4], argv[5]);
	}
	else{
		std::cout<<"error usage!"<<std::endl;
		exit(-1);
	}
}
