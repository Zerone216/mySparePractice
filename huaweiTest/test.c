/*
 > Declaration: 
 >  Copyright (c) 2009-2019 Zhang Sheng <zerone216@163.com>
 >  See my homepage: https://zerone216.cn
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
 > FilePath: /zerone216/mySparePractice/huaweiTest/test.c
 > CreateTime: 2019-12-24 22:43:14
 > ModifyTime: 2019-12-24 22:44:31
 > LastEditors: 
 > Description: 
 > version: 
 > Repository: https://github.com/zerone216/
 > Support: 
 > Others: 
 > Changelogs: 
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define YES 0x01
#define NO 0x00

#define TRUE 0x01
#define FALSE 0x00

#define malloc_type(type, num) (type *)malloc(sizeof(type) * (num))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

#define SIZE_1KB 1024ULL
#define SIZE_1MB (SIZE_1KB * 1024)
#define SIZE_1GB (SIZE_1MB * 1024)
#define SIZE_1TB (SIZE_1GB * 1024)

struct _DiskSize
{
    char expSize[32];
    unsigned long long realSize;
};
typedef struct _DiskSize DiskSize; 

static char * string_dup(const char * src, int size)
{
    if(!src)
        return NULL;
    
    int len = strlen(src);
    if(len < 0 || len > 10 * SIZE_1MB)
        return NULL;
    
    char * str = malloc_type(char, size);
    if(!src)
        return NULL;
    
    memset(str, 0x00, size);
    memcpy(str, src, MIN(len, (size - 1)));
    
    return str;
}

static void string_free(char ** pstr)
{
    if(*pstr)
    {
        free(*pstr);
        *pstr = NULL;
    }
}

static unsigned long long analyseExpToRealSize(const char * exp)
{
    if(!exp)
        return -1;
    
    unsigned long long realsize = 0;
    int len = strlen(exp);
    char flag = exp[len -1];
    char sizetmp[32] = {'\0'};
    memcpy(sizetmp, exp, len - 1);
    
    switch(flag)
    {
        case 'M':
            realsize = atoi(sizetmp) * SIZE_1MB;
            break;
        case 'G':
            realsize = atoi(sizetmp) * SIZE_1GB;
            break;
        case 'T':
            realsize = atoi(sizetmp) * SIZE_1TB;
            break;
        default:
            realsize = -1;
            break;
    }
    
    return realsize;
}

static int memSwap(unsigned char * a, unsigned char * b, int blksize)
{
    if(!a || !b)
        return -1;
    
    unsigned char * tmp = malloc_type(unsigned char, blksize);
    if(!tmp)
        return -1;
    
    memcpy(tmp, a, blksize);
    memcpy(a, b, blksize);
    memcpy(b, tmp, blksize);
    
    free(tmp);
    tmp = NULL;
    return 0;    
}

int bubleSort(DiskSize * disksize, int len)
{
    if(!disksize || len < 0)
        return -1;
    
    if(0 == len)
        return 0;
    
    int i = 0, j = 0;
    for(i = 0; i < len; i ++)
    {
        for(j = i + 1; j < len; j ++)
        {
            if(disksize[i].realSize < disksize[j].realSize)
                memSwap((unsigned char *)&disksize[i], (unsigned char *)&disksize[j], sizeof(DiskSize));
        }
    }
    
    return 0;
}

int main(int argc, const char ** argv)
{
    int disknum = 0;   
    
    scanf("%d", &disknum);
    DiskSize * disksize = malloc_type(DiskSize, disknum);
    if(!disksize)
        return 1;
    
    memset(disksize, 0x00, disknum *sizeof(DiskSize));
    int i = 0;
    
    while(i < disknum)
    {
        memset(disksize[i].expSize, 0x00, sizeof(disksize[i].expSize));
        scanf("%s", disksize[i].expSize);
        
        disksize[i].realSize = analyseExpToRealSize(disksize[i].expSize);
        //if(disksize.realSize < 0)
        
        
        ++ i;
    }
    
    if(bubleSort(disksize, disknum)== 0)
    {
        for(i = 0; i <disknum; i ++)
            printf("%s\n", disksize[i].expSize);
    }
    
    free(disksize);
    disksize = NULL;
    
    return 0;
}