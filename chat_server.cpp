#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "common.h"
#include "file.h"


typedef struct tpstClientInfo
{
    int iFd;
    int iPort;
    char sUsername[COMMON_NAME_BUFF];
    char sIP[COMMON_IP_BUFF];
}TPST_CLIENT_INFO;

int getClientFd(char *caName);
bool getNameFromFd(int iFd, char *pcName);

map<int, TPST_CLIENT_INFO> g_mpClientInfo;

int main(int argc, char *argv[])
{
	int server_sockfd, client_sockfd;
	unsigned int server_len, client_len;
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;
	int result;
	list<int> lsFd;
	fd_set readfds, testfds;
	TPST_FILE_CONF stConf = {0};
    char recvline[COMMON_MSG_BUFF] = {0};
    char sendline[COMMON_MSG_BUFF] = {0};
    char caMsg[COMMON_MSG_BUFF] = {0};

	assert(argc == 2);
	file_readConf(stConf, argv[1]);
	//printf("DEBUG read port: %d\n", stConf.iPort);

	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);//create socket
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);//to network byte order
	server_address.sin_port = htons(stConf.iPort);

	server_len = sizeof(server_address);

	bind(server_sockfd, (struct sockaddr *)&server_address, server_len);
	listen(server_sockfd, 5);

    getsockname(server_sockfd, (struct sockaddr*)&server_address, &server_len);
    int port=ntohs(server_address.sin_port);
    printf("Host: %s\tPort: %d\n", getenv("HOSTNAME"), port);
    //printf("DEBUG get process port is: %d\n", port);
	FD_ZERO(&readfds);
	FD_SET(server_sockfd, &readfds);
	lsFd.push_back(server_sockfd);
	//printf("DEBUG add server fd: %d\n", server_sockfd);
	while(1)
	{

		int nread;
		testfds = readfds;
		//printf("DEBUG server waiting\n");

		/*check ready fds */
		result = select(FD_SETSIZE, &testfds, (fd_set *)0,(fd_set *)0, (struct timeval *) 0);
		if(result < 1)
		{
			perror("Error");
			exit(1);
		}

		/*loop through every fd*/
		for(auto tpFd: lsFd)
		{
            // if fd is set    
			if (FD_ISSET(tpFd, &testfds))
            {
                //if there is a new connection to server
                if(tpFd == server_sockfd)
                {
                    client_len = sizeof(client_address);
                    client_sockfd = accept(server_sockfd,
                    (struct sockaddr *)&client_address, &client_len);
                    FD_SET(client_sockfd, &readfds);//add client to fd list
          //          printf("DEBUG adding client on fd %d\n", client_sockfd);

                    lsFd.push_back(client_sockfd);
                }
                else
                {
                    //check if fd is close
                    ioctl(tpFd, FIONREAD, &nread);

                    
                    if(nread == 0)
                    {
                        close(tpFd);
                        FD_CLR(tpFd, &readfds);
                        //printf("DEBUG removing client on fd %d\n", tpFd);
                        lsFd.remove(tpFd);
                        /* remove item of list, break loop */
                        break;
                    }

                    //new message 
                    else
                    {
       
                        int res = 0;
                        int iRecLen = 0;
                        char caClientName[COMMON_NAME_BUFF] = {0};
                        int iPort = 0;

                        memset(recvline, 0, sizeof(recvline));
                        iRecLen = read(tpFd, recvline, sizeof(recvline));
                        if(0 > iRecLen)
                        {
                            perror("Failed to read\n");
                        }
                        //printf("DEBUG receive: %s len: %d\n", recvline, iRecLen);

                        memset(caMsg, 0, sizeof(caMsg));

                        res = sscanf(recvline, "chat @%s %s",
                                     caClientName, caMsg);
                        if (res > 0)
                        {
                            int iDesFd = 0;
                            bool bTpRes = false;
                            int iPreLen = 0;
                            /* hint */
                            iPreLen = strlen("chat @") + strlen(caClientName) + 1;
                            /* ignore space in message */
                            strncpy(caMsg, recvline + iPreLen, strlen(recvline) - iPreLen);

                            iDesFd = getClientFd(caClientName);
                            //printf("DEBUG send to user %s msg: %s found fd: %d\n",
                            //       caClientName, caMsg, iDesFd);
                            assert(0 != iDesFd);
                            if (iDesFd == tpFd)
                            {
                                /* self message */
                                assert(0);
                            }
                            memset(caClientName, 0, sizeof(caClientName));
                            bTpRes = getNameFromFd(tpFd, caClientName);
                            assert(bTpRes);
                            memset(sendline, 0, sizeof(sendline));
                            sprintf(sendline, "%s >> %s\n", caClientName, caMsg);
                            write(iDesFd, sendline, sizeof(sendline));
                            continue;
                        }

                        res = sscanf(recvline, "chat %s",
                                     caMsg);
                        if (res > 0)
                        {
                            /* hint */
                            bool bTpRes = false;

                            int iPreLen = strlen("chat ");
                            /* ignore space in message */
                            strncpy(caMsg, recvline + iPreLen, strlen(recvline) - iPreLen);

                            //printf("DEBUG send to all users msg: %s\n",
                            //       caMsg);

                            memset(caClientName, 0, sizeof(caClientName));
                            bTpRes = getNameFromFd(tpFd, caClientName);
                            assert(bTpRes);

                            memset(sendline, 0, sizeof(sendline));
                            sprintf(sendline, "%s >> %s", caClientName, caMsg);
                            for (auto stNode: g_mpClientInfo)
                            {
                                int iDesFd = stNode.second.iFd;
                                assert(0 != iDesFd);
                                write(iDesFd, sendline, sizeof(sendline));
                            }
                            continue;
                        }

                        res = sscanf(recvline, "get %s",
                                     caClientName);
                        if (res > 0)
                        {
                            int iDesFd = 0;
                            bool bTpRes = false;
                            /* hint */
                            iDesFd = getClientFd(caClientName);
                            //printf("DEBUG send to user %s found fd: %d\n",
                            //       caClientName, iDesFd);
                            assert(0 != iDesFd);
                            if (iDesFd == tpFd)
                            {
                                /* self message */
                                assert(0);
                            }
                            memset(caClientName, 0, sizeof(caClientName));
                            bTpRes = getNameFromFd(tpFd, caClientName);
                            assert(bTpRes);
                            memset(sendline, 0, sizeof(sendline));
                            sprintf(sendline, "info %s %d self: %s\n",
                                    g_mpClientInfo[iDesFd].sIP,
                                    g_mpClientInfo[iDesFd].iPort, caClientName);
                            write(tpFd, sendline, sizeof(sendline));
                            continue;
                        }

                        res = sscanf(recvline, "login %s port: %d",
                                     caClientName, &iPort);
                        if (res > 0)
                        {
                            struct sockaddr_in sa;
                            unsigned int len;
                            len = sizeof(sa);

                            /* hint */
                            if (g_mpClientInfo.find(tpFd) != g_mpClientInfo.end())
                            {
                                /* has login */
                                write(tpFd, "has login, ignore", sizeof("has login, ignore"));
                                continue;
                            }
                            g_mpClientInfo[tpFd].iFd = tpFd;
                            g_mpClientInfo[tpFd].iPort = iPort;
                            strncpy(g_mpClientInfo[tpFd].sUsername,
                                    caClientName,
                                    sizeof(g_mpClientInfo[tpFd].sUsername));
                            if(!getpeername(tpFd, (struct sockaddr *)&sa, &len))
                            {
                                strncpy(g_mpClientInfo[tpFd].sIP,
                                        inet_ntoa(sa.sin_addr),
                                        sizeof(g_mpClientInfo[tpFd].sIP));
                                printf("User %s login fd: %d ip: %s port: %d\n",
                                       caClientName, tpFd,
                                       g_mpClientInfo[tpFd].sIP, g_mpClientInfo[tpFd].iPort);
                            }

                            write(tpFd, COMMON_LOGIN_OK, sizeof(COMMON_LOGIN_OK));
                            continue;
                        }

                        if (strncasecmp(recvline, "logout", sizeof("logout")) == 0)
                        {
                            /* logout */
                            printf("User id: %d logout\n", tpFd);
                            g_mpClientInfo.erase(tpFd);
                            write(tpFd, COMMON_LOGOUT_OK, sizeof(COMMON_LOGOUT_OK));
                            break;
                        }

                    }
                }
            }

		}
	}
}
/*****************
input:
        char *caName
description:
        find fd by name
******************/
int getClientFd(char *caName)
{
    for (auto stNode: g_mpClientInfo)
    {
        if (strncasecmp(stNode.second.sUsername, caName, sizeof(stNode.second.sUsername)) == 0)
        {
            /* found */
            return stNode.second.iFd;
        }
    }

    return 0;
}
/*****************
input:
        int iFd
        char *pcName
description:
        find name by id
******************/
bool getNameFromFd(int iFd, char *pcName)
{
    if (g_mpClientInfo.find(iFd) != g_mpClientInfo.end())
    {
        strncpy(pcName, g_mpClientInfo[iFd].sUsername, sizeof(g_mpClientInfo[iFd].sUsername));
        return true;
    }

    return false;
}
