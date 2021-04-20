#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "common.h"
#include "file.h"

int g_sockfd;
int g_serverPort;

bool g_bNewMsg;
char g_recvline[COMMON_MSG_BUFF];
char g_sDesIP[COMMON_IP_BUFF];
char g_sSelfName[COMMON_NAME_BUFF];
int g_iDesPort;

enum CLIENT_STATE
{
      E_CSTATE_IDLE=1, E_CSTATE_LOGIN
};

enum CLIENT_STATE g_eCState;
void* myRecv(void* arg);        //receiver
void* myServer(void* arg);        //server
int mySend(char * pcMsg);

bool myExit()
{
    if (E_CSTATE_IDLE == g_eCState)
    {
        printf("SYSTEM Exit successfully\n");
        close(g_sockfd);
        return true;
    }
    else
    {
        printf("SYSTEM Please logout first\n");
    }
    return false;
}
void sigroutine(int dunno)
{
    switch (dunno)
    {
    case 1:
        printf("Get a signal -- SIGHUP\n");
        break;
    case 2:
        //printf("Get a signal -- SIGINT\n");
        if (myExit())
        {
            exit(0);
        }
        break;
    case 3:
        printf("Get a signal -- SIGQUIT\n");
        break;
    }
    //printf("Get a signal\n");
    return;
}

int main(int argc, char *argv[])
{
    int sockfd = 0;
    char sendline[COMMON_MSG_BUFF] = {0};
    struct sockaddr_in servaddr;

	TPST_FILE_CONF stConf = {0};
	g_eCState = E_CSTATE_IDLE;
	g_bNewMsg = false;
    pthread_t  connectthread;	   //thread

	assert(argc == 2);
	file_readConf(stConf, argv[1]);
	//printf("SYSTEM read IP: %s port: %d\n", stConf.sIP, stConf.iPort);

	/* handle signal */
	signal(SIGINT, sigroutine);

    //create socket
    if( (sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
        printf(" create socket error: %s (errno :%d)\n",strerror(errno),errno);
        return 0;
    }

    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(stConf.iPort);
    //conver ip address to binary form
    if( inet_pton(AF_INET, stConf.sIP, &servaddr.sin_addr) <=0 ) {
        printf("inet_pton error for %s\n",argv[1]);
        return 0;
    }
    //connect to server
    if( connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) <0) {
        printf(" connect socket error: %s(errno :%d)\n",strerror(errno),errno);
        return 0;
    }

    g_sockfd = sockfd;

    //create thread for myRecv to receive message from server
    if (pthread_create(&connectthread, NULL, myRecv, (void*)&sockfd))
    {
        printf("Pthread_create() error");
        exit(0);
    }
    //create thread for my server to receive message from users
    if (pthread_create(&connectthread, NULL, myServer, (void*)&sockfd))
    {
        printf("Pthread_create() error");
        exit(0);
    }

    //printf("send msg to server:\n");
    while(1)
	{
		fgets(sendline,sizeof(sendline),stdin);
		sendline[strlen(sendline)-1] = '\0';

		/* ignore case */
		if(strncasecmp(sendline, "exit", sizeof("exit")) == 0)
		{
		    if (myExit())
            {
                break;
            }
            else
            {
                continue;
            }
		}
		/* send message to user */
		if(strncasecmp(sendline, "chat @", strlen("chat @")) == 0)
		{
		    mySend(sendline);
		    continue;
		}
		/* append information to chat_server*/
		if(strncasecmp(sendline, "login ", strlen("login ")) == 0)
		{
            char caTpMsg[COMMON_MSG_BUFF] = {0};
            sprintf(caTpMsg, "%s port: %d ", sendline, g_serverPort);
            //printf("DEBUG send: %s\n", caTpMsg);
            memset(sendline, 0, sizeof(sendline));
            sprintf(sendline, "%s", caTpMsg);
		}

		int sentbytes = send(sockfd,sendline,strlen(sendline),0);
        if(sentbytes < 0)
        {
            perror("Error: failed to send message\n");
        }
	}
    //close(sockfd);
    return 0;
}
/*****************
input:
        void* arg socket fd
description:
        receive message from server and print
******************/
void* myRecv(void* arg)
{
    int sockfd = *((int *)arg);

    while(1)
    {
        int iReLen = 0;
        int res = 0;
        memset(g_recvline, 0, sizeof(g_recvline));
        iReLen = recv(sockfd, g_recvline, sizeof(g_recvline), 0);
        if(iReLen < 0)
        {
            perror("Failed to receive\n");
        }
        g_recvline[iReLen] = '\0';
        //printf("recv msg from server: %s\n", g_recvline);
        if (strncasecmp(g_recvline, COMMON_LOGIN_OK, sizeof(COMMON_LOGIN_OK)) == 0)
		{
            g_eCState = E_CSTATE_LOGIN;
            printf("SYSTEM %s\n", COMMON_LOGIN_OK);
            continue;
		}
        if (strncasecmp(g_recvline, COMMON_LOGOUT_OK, sizeof(COMMON_LOGOUT_OK)) == 0)
		{
            g_eCState = E_CSTATE_IDLE;
            printf("SYSTEM %s\n", COMMON_LOGOUT_OK);
            continue;
		}
        res = sscanf(g_recvline, "info %s %d self: %s", g_sDesIP, &g_iDesPort, g_sSelfName);
        if (res > 0)
        {
            g_bNewMsg = true;
            continue;
        }
        printf("%s\n", g_recvline);

    }

    pthread_exit(NULL);
}
/*****************
input:
        char * pcMsg
description:
        send message and print
******************/
int mySend(char * pcMsg)
{

    char sendline[COMMON_MSG_BUFF] = {0};
    char caMessage[COMMON_MSG_BUFF] = {0};
    char caName[COMMON_NAME_BUFF] = {0};

    int res = 0;
   
    int iPreLen = 0;

    //printf("DEBUG my send\n");

    res = sscanf(pcMsg, "chat @%s %s", caName, caMessage);
    iPreLen = strlen("chat @") + strlen(caName) + 1;
    /* ignore space in message */
    strncpy(caMessage, pcMsg + iPreLen, strlen(pcMsg) - iPreLen);
    if (res > 0)
    {//send msg to other user
		int iLen = 0;
        int sockfd = 0;
        struct sockaddr_in servaddr;

		sprintf(sendline, "get %s", caName);
		iLen = send(g_sockfd,sendline,strlen(sendline),0);
        if(iLen < 0)
        {
            perror("Failed to receieve\n");
        }
		while (!g_bNewMsg)
        {
            /* wait until new message g_recvline come */
        }

        g_bNewMsg = false;

        //socket
        if( (sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
            printf(" create socket error: %s (errno :%d)\n",strerror(errno),errno);
            return 0;
        }

        memset(&servaddr,0,sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(g_iDesPort);
        //convert ip to binary
        if( inet_pton(AF_INET, g_sDesIP, &servaddr.sin_addr) <=0 ) {
            printf("inet_pton error for %s\n", g_sDesIP);
            return 0;
        }
        //connect
        if( connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) <0) {
            printf(" connect socket error: %s(errno :%d)\n",strerror(errno),errno);
            return 0;
        }

        //printf("send msg to server:\n");
        //send message
        memset(sendline, 0, sizeof(sendline));
        sprintf(sendline, "%s >> %s", g_sSelfName, caMessage);
        if ( send(sockfd,sendline,strlen(sendline),0) <0) {
            printf("send msg error: %s(errno :%d)\n",strerror(errno),errno);
            return 0;
        }
        printf("SYSTEM send OK!\n");

        close(sockfd);
    }

    return 0;
}
/*****************
input:
        void* arg NULL
description:
        receive message from other users and print
******************/
void* myServer(void* arg)
{
	int listenfd;				   //socket
    struct sockaddr_in server;     //
	unsigned int server_len;
    uint16_t port = 0;

    //create socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Creating socket failed.");
        assert(0);
    }

    bzero(&server,sizeof(server));
    server.sin_family=AF_INET;
    server.sin_port=htons(port);
    server.sin_addr.s_addr = htonl (INADDR_ANY);

	server_len = sizeof(struct sockaddr_in);
    //bind
    if (bind(listenfd, (struct sockaddr *)&server, server_len) == -1) {
        printf("Bind error.");
        assert(0);
    }
    //listen
    if(listen(listenfd, 5) == -1){
        printf("listen() error\n");
        assert(0);
    }

    getsockname(listenfd, (struct sockaddr*)&server, &server_len);
    g_serverPort = ntohs(server.sin_port);

    while(1)
    {
        int connectfd;       		   //socket
        int num;
        char recvbuf[COMMON_MSG_BUFF];
        struct sockaddr_in client;     //
        int sin_size = sizeof(struct sockaddr_in);

        //accept connection
        if ((connectfd = accept(listenfd,(struct sockaddr *)&client,(socklen_t *)&sin_size))==-1) {
            printf("accept() error\n");
            assert(0);
        }

        //printf("You got a connection from %s.  \n",inet_ntoa(client.sin_addr) );
        //MSG_WAITALL
        while ((num = recv(connectfd, recvbuf, COMMON_MSG_BUFF,0)) > 0) {
            recvbuf[num] = '\0';
            printf("%s\n", recvbuf);
        }
        //printf("Disconnected.\n");
        close(connectfd);
    }

	//close socket
    close(listenfd);

    pthread_exit(NULL);
}
