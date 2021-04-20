/***************
input file output file function
****************/
#include "common.h"
#include "file.h"

/*****************
input:
        TPST_FILE_CONF &stConf, char *caFile
description:
        read configuration file and fill the list g_initG
******************/
int file_readConf(TPST_FILE_CONF &stConf, char *caFile)
{
    FILE *fp = NULL;

    char caBuffer[FILE_ONE_LINE_BUFFER] = {0};
    int iCnt = 0;

    assert(NULL != caFile);

    fp = fopen(caFile, "rt");

    if (NULL == fp)
    {
        printf("error no file: %s\n", caFile);
        assert(0);
    }
    while (fgets(caBuffer, FILE_ONE_LINE_BUFFER - 1, fp) > 0)
    {
        int res = 0;
        res = sscanf(caBuffer, "port:%d", &(stConf.iPort));
        if (res > 0)
        {
            ++iCnt;
            continue;
        }
        res = sscanf(caBuffer, "servport:%d", &(stConf.iPort));
        if (res > 0)
        {
            ++iCnt;
            continue;
        }
        res = sscanf(caBuffer, "servhost:%s", (char *)&(stConf.sIP));
        if (res > 0)
        {
            ++iCnt;
            continue;
        }
    }
    fclose(fp);
    return iCnt;
}
