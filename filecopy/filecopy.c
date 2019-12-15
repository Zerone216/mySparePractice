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

#define BLOCK_BUFF_SIZE (1024 * 1024)

typedef struct _File File;
struct _File
{
    char * filename;
    int fd;
    unsigned long long filesize;
    unsigned long long curposition;

   /*  int (*open)(File *, const char *, const int);
    int (*create)(File *, const char *);
    void (*close)(File *);
    long long (*seek)(File *, long long, int);
    int (*read)(File *, long long, unsigned char * , int);
    int (*write)(File *, long long, unsigned char * , int); */
};

static char * string_dup(const char * src, int assignlen)
{
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
        return 0;
    }

    return 1;
}

static display_file_info(File * file)
{
    printf("============================");

    printf("filename=[%s]\n", file->filename);
    printf("fd=[%d]\n", file->fd);
    printf("filesize=[%lld]\n", file->filesize);
    printf("curposition=[%lld]\n", file->curposition);

    printf("============================");
}

File * initFile(const char * filename, long long offset)
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
        if((file->fd = fileCreate(file->filename)) == -1)
        {
            goto err1;
        }
    }
    else
    {
        if((file->fd = fileOpen(file->filename, O_RDWR)) == -1)
        {
            goto err1;
        }
    }

    file->filesize = lseek64(file->fd, 0, SEEK_END);
    if(file->filesize < 0)
    {
        goto err1;
    }

    if((file->curposition = fileSeek(file->fd, offset, SEEK_SET)) < 0)
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

int fileCopy()
{
    unsigned char * blkBuf = (unsigned char *) malloc(BLOCK_BUFF_SIZE);
    return 0;
}

int main(int argc, const char** argv) {

    /* char * str = string_dup(argv[1], atoi(argv[2]));
    printf("str=[%s]\n", str);
    string_free(&str); */


    File * file = initFile(argv[1], atoi(argv[2]));
    if(!file)
        return 1;

    display_file_info(file);
    destroyFile(&file);
    return 0;
}
