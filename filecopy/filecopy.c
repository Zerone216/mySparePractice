/*
 > Declaration: 
 >  Copyright (c) 2009-2019 Zhang Sheng <zerone216@163.com>
 >  See my homepage: http://www.zerone216.top:8080
 > 
 >  This program is free software; you can redistribute it and/or modify it under 
 >  the terms of the GNU General Public License as published by the Free Software 
 >  Foundation, either version 2 or any later version.
 >  
 >  Redistribution and use in source and binary forms, with or without modification,
 >  are permitted provided that the following conditions are met: 
 >  
 >  1. Redistributions of source code must retain the above copyright notice, this
 >  list of conditions and the following disclaimer.
 >  
 >  2. Redistributions in binary form must reproduce the above copyright notice,
 >  this list of conditions and the following disclaimer in the documentation and/or 
 >  other materials provided with the distribution. 
 > 
 >  This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 >  WARRANTY; without even the implied warranty of MERCHANTABILITY or ITNESS FOR A 
 >  PARTICULAR PURPOSE. See the GNU General Public License for more details. A copy 
 >  of the GNU General Public License is available at: 
 >  http://www.fsf.org/licensing/licenses 
 >  
 >  All manufacturing, reproduction, use, and sales rights pertaining to this subject 
 >  matter are governed bythe license agreement. The recipient of this software 
 >  implicitly accepts the terms of the license.
 > 
 > Author: Zhang Sheng
 > FilePath: \22g:\IDV\filecopy.c
 > CreateTime: 2019-12-13 10:32:11
 > ModifyTime: 2019-12-13 10:32:14
 > LastEditors: 
 > Description: 
 > version: 
 > Repository: https://github.com/zerone216/
 > Support: 
 > Others: 
 > Changelogs: 
 */

#define __USE_FILE_OFFSET64
#define __USE_LARGEFILE64
#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <regex.h>
#include<sys/types.h>
#include<sys/stat.h>

#define TRUE 1
#define FALSE 0

#define YES 1
#define NO 0

#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

//定义flags:读写，文件不存在那么就创建，文件长度戳为0
#define FLAGS (O_RDWR | O_CREAT | O_TRUNC)
//创建文件的权限，用户读、写、执行、组读、执行、其他用户读、执行
#define MODE (S_IRWXU | S_IXGRP | S_IROTH | S_IXOTH)

#define BLOCK_BUFF_MAX_SIZE (10 * 1024 * 1024)

typedef struct _File File;
struct _File
{
    char * filename;
    int fd;
    unsigned long long filesize;
    unsigned long long start;
    unsigned long long curoffset;
};


typedef struct _FileCopy FileCopy;
struct _FileCopy
{
    char * inputFile;
    long long readoffset;
    char * outputFile;
    long long writeoffset;
    long long datasize;
    int clean;
};

#define OPTION_COUNT 6

typedef enum _OPTION {
    INPUTFILE = 0,
    READOFFSET,
    OUTPUTFILE,
    WRITEOFFSET,
    DATASIZE,
    CLEAN
}OPTION;

const char * option[] = {
    "if=",
    "ro=",
    "of=",
    "wo=",
    "ds=",
    "cl="
};

static char * string_dup(char * src, int assignlen)
{
    if(src == NULL)
    {
        return NULL;
    }

    int len = strlen(src);
    if(len <= 0 || len > 10485760)
    {
        return NULL;
    }

    int size = (assignlen <= 0) ? (len + 1) : (assignlen);   
    char * str = (char *) malloc(size);
    if(!str)
        return NULL;

    memset(str, 0x00, size);
    if(len)
        memcpy(str, src, MIN(len, size - 1));

    return str;
}

static void string_free(char ** string)
{
    if(*string)
    {
        free(*string);
        *string = NULL;
    }
}

static int fileOpen(const char * filename, const int mode)
{
    if(NULL == filename)
    {
        return -1;
    }

    int fd = open(filename, mode);
    if(fd <= 0)
        return -1;

    return fd;
}

static int fileCreate(const char * filename)
{
    int fd = open(filename, FLAGS, MODE);
    if(fd <= 0)
        return -1;

    return fd;
}

static void fileClose(int * fd)
{
    if(*fd)
    {
        close(*fd);
        *fd = 0;
    }
}

static long long fileSeek(int fd, long long whereis, int tag)
{
    return lseek64(fd, whereis, tag);
}

static int fileRead(int fd, long long offset, unsigned char * buf, int size)
{
    if(NULL == buf || size <= 0)
    {
        return -1;
    }

    if(fileSeek(fd, offset, SEEK_SET) < 0)
    {
        return -1;
    }

    if(read(fd, buf, size) != size)
    {
        return -1;
    }

    return 0;
}

static int fileWrite(int fd, long long offset, unsigned char * buf, int size)
{
    if(NULL == buf || size <= 0)
    {
        return -1;
    }

    if(fileSeek(fd, offset, SEEK_SET) < 0)
    {
        return -1;
    }

    if(write(fd, buf, size) != size)
    {
        return -1;
    }
    
    return 0;
}

static int fileAccess(const char *filename, int ftag)
{
    if(access(filename, ftag) < 0)
    {
        return 1;
    }

    return 0;
}

static void display_file_info(File * file)
{
    printf("============================\n");

    printf("filename=[%s]\n", file->filename);
    printf("fd=[%d]\n", file->fd);
    printf("filesize=[%lld]\n", file->filesize);
    printf("start=[%lld]\n", file->start);
    printf("curoffset=[%lld]\n", file->curoffset);

    printf("============================\n");
}

File * initFile(char * filename, long long offset)
{
    File * file = (File *) malloc(sizeof(File));
    if(!file)
    {
        return NULL;
    }

    memset(file, 0x00, sizeof(File));
    file->filename = string_dup(filename, 256);
    if(!file->filename)
    {
        goto err;
    }

    if(fileAccess(file->filename, F_OK) == 1)
    {
        printf("file[%s] is not existed, need to create it!\n", file->filename);
        if((file->fd = fileCreate(file->filename)) == -1)
        {
            goto err1;
        }

        file->filesize = 0;
    }
    else
    {
        if((file->fd = fileOpen(file->filename, O_RDWR)) == -1)
        {
            goto err1;
        }

        file->filesize = lseek64(file->fd, 0, SEEK_END);
        if(file->filesize < 0)
        {
            goto err1;
        }
    }

    file->start = offset;
    if((file->curoffset = fileSeek(file->fd, file->start, SEEK_SET)) < 0)
    {
        goto err1;
    }

    return file;

err1:
    free(file->filename);

err:
    free(file);

    return NULL;
}

void destroyFile(File ** file)
{
    if(*file)
    {
        if((*file)->fd)
        {
            fileClose(&(*file)->fd);
        }

        if((*file)->filename)
        {
            free((*file)->filename);
            (*file)->filename = NULL;
        }

        free(*file);
    }
}

int doFileCopy(char * inputFile, long long skip, char * outputFile, long long seek, long long copySize, int clean)
{
    if(inputFile == NULL || strlen(inputFile) == 0)
    {
        printf("Error: Invalid parameter, input file name is empty.\n");
        return -1;
    }

    if(outputFile == NULL || strlen(outputFile) == 0)
    {
        printf("Error: Invalid parameter, output file name is empty.\n");
        return -1;
    }

    if(skip < 0)
    {
        printf("Error: Invalid parameter, skip[%lld] is less than zero.\n", skip);
        return -1;
    }

    if(seek < 0)
    {
        printf("Error: Invalid parameter, seek[%lld] is less than zero.\n", seek);
        return -1;
    }

    if(copySize == 0) //nothing here
    {
        printf("Warning: copySize[%lld] is equal to zero, do nothing.\n", copySize);
        return 0;
    }

    File * infile = initFile(inputFile, skip);
    if(!infile)
    {
        goto err;
    }
    //display_file_info(infile);

    File * outfile = initFile(outputFile, seek);
    if(!outfile)
    {
        goto err1;
    }
    //display_file_info(outfile);
    if(clean)
        ftruncate(outfile->fd, 0);

    if(copySize < 0)
        copySize = infile->filesize - infile->start;
    else if(copySize + infile->start > infile->filesize)
    {
        printf("Warning: copySize[%lld] is too big if copy from offset[%lld].\n", copySize, infile->start);
        copySize = infile->filesize - infile->start;
    }
    
    int blockSize = MIN(copySize, BLOCK_BUFF_MAX_SIZE); //MAX 10MB
    printf("copySize=[%lld], blockSize=[%d]\n", copySize, blockSize);

    long long count = (copySize + blockSize - 1) / blockSize; //at least 1 block
    printf("count=[%lld]\n", count);

    unsigned char * blockBuf = (unsigned char *) malloc(blockSize);
    if(NULL == blockBuf)
    {
        printf("malloc buf failed!");
        goto err2;
    }

    int i = 0;
    for(i = 0; i < count; i ++)
    {
        int optSize = MIN(copySize - i * blockSize, blockSize);        
        if(fileRead(infile->fd, infile->curoffset, blockBuf, optSize) < 0)
        {
            printf("fileRead file[%s:%d] block[%d] curoffset[%llu] optSize[%d] filed\n", infile->filename, infile->fd, i, infile->curoffset, optSize);
            goto err3;
        }
        infile->curoffset += optSize;

        if(fileWrite(outfile->fd, outfile->curoffset, blockBuf, optSize) < 0)
        {
            printf("fileWrite file[%s:%d] block[%d] curoffset[%llu] optSize[%d] filed\n", infile->filename, infile->fd, i, infile->curoffset, optSize);
            goto err3;
        }
        outfile->curoffset += optSize;
    }

    fsync(outfile->fd);

    free(blockBuf);
    blockBuf = NULL;
    destroyFile(&outfile);
    destroyFile(&infile);
    return 0;

err3:
    free(blockBuf);
    blockBuf = NULL;

err2:
    destroyFile(&outfile);

err1:
    destroyFile(&infile);

err:
    return -1;
}

char * getOptionVal(char * argv, OPTION * opt)
{
    if(!argv || !opt)
        return NULL;
    
    int len = 0;
    *opt = -1;
    char * value = NULL;
    int i = 0;
    for(i = 0; i < OPTION_COUNT; i ++)
    {
        len = strlen(option[i]);
        if(strncmp(argv, option[i], len) == 0)
        {
            *opt = i;
            value = argv + len;
            break;
        }
    }

    return value;
}

int parse_argvs(int argc, char ** argv, FileCopy * fileCopy)
{
    int i = 1;
    char * value = NULL;
    OPTION opt = -1;

    for(i = 1; i < argc; ++ i)
    {
        value = getOptionVal((char *)argv[i], &opt);
        if(!value)
            continue;
        
        switch(opt) {
            case INPUTFILE:
                fileCopy->inputFile = string_dup(value, 256); //strncpy(fileCopy->inputFile, value, strlen(value)); //
                //printf("inputFile=[%s]\n", fileCopy->inputFile);
                break;
            
            case READOFFSET:
                fileCopy->readoffset = atoll(value);
                //printf("readoffset=[%lld]\n", fileCopy->readoffset);
                break;

            case OUTPUTFILE:
                fileCopy->outputFile = string_dup(value, 256); //strncpy(fileCopy->outputFile, value, strlen(value)); //
                //printf("outputFile=[%s]\n", fileCopy->outputFile);
                break;

            case WRITEOFFSET:
                fileCopy->writeoffset = atoll(value);
                //printf("writeoffset=[%lld]\n", fileCopy->writeoffset);
                break;

            case DATASIZE:
                fileCopy->datasize = atoll(value);
                //printf("datasize=[%lld]\n", fileCopy->datasize);
                break;
            
            case CLEAN:
                fileCopy->clean = (strncmp(value,"yes", strlen("yes")) == 0) ? 1 : ((strncmp(value,"y", strlen("y")) == 0) ? 1 : 0);
                //printf("clean=[%d]\n", fileCopy->clean);
                break;
            
            default:
                break;
        }
    }

    return 0;
}

static void displayUsage()
{
    printf("\e[38;5;9m\e[1mUsage:\n\e[0m");
    printf("\e[38;5;11m\e[1m    ./filecopy ");
    printf("\e[38;5;14mif=[");
    printf("\e[38;5;10minputfile");
    printf("\e[38;5;14m] ro=[");
    printf("\e[38;5;10mreadoffset");
    printf("\e[38;5;14m] of=[");
    printf("\e[38;5;10moutputfile");
    printf("\e[38;5;14m] wo=[");
    printf("\e[38;5;10mwriteoffset");
    printf("\e[38;5;14m] ds=[");
    printf("\e[38;5;10mdatasize");
    printf("\e[38;5;14m]\n\n\e[1m\e[0m");
}

int main(int argc, const char** argv) {

    /* char * str = string_dup(argv[1], atoi(argv[2]));
    printf("str=[%s]\n", str);
    string_free(&str); */


    /* File * file = initFile(argv[1], atoi(argv[2]));
    if(!file)
        return 1;

    display_file_info(file);
    destroyFile(&file); */

    if(argc < 2)
    {
        displayUsage();
        return 1;
    }

    FileCopy fileCopy;
    memset(&fileCopy, 0x00, sizeof(FileCopy));
    parse_argvs(argc, (char **)argv, &fileCopy);

    doFileCopy(fileCopy.inputFile, fileCopy.readoffset, fileCopy.outputFile, fileCopy.writeoffset, fileCopy.datasize, fileCopy.clean);

    string_free(&fileCopy.inputFile);
    string_free(&fileCopy.outputFile);
    //doFileCopy(argv[1], atoll(argv[2]), argv[3], atoll(argv[4]), atoll(argv[5]));
    return 0;
}
