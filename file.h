#pragma once
/***************
input file output file function
****************/

#define FILE_ONE_LINE_BUFFER 100
#define FILE_NAME_BUFFER 100
#define FILE_IP_BUFFER 20

typedef struct tpstFileConf
{
    int iPort;
    char sIP[FILE_IP_BUFFER];
}TPST_FILE_CONF;

int file_readConf(TPST_FILE_CONF &stConf, char *caFile);
