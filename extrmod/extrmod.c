/*
 * @Declaration: 
 *  Copyright (c) 2009-2019 Zhang Sheng (Mr. Zhang) 
 * 
 *  This program is free software; you can redistribute it and/or modify it under 
 *  the terms of the GNU General Public License as published by the Free Software 
 *  Foundation, either version 2 or any later version. 
 *  
 *  Redistribution and use in source and binary forms, with or without modification, 
 *  are permitted provided that the following conditions are met: 
 *  
 *  1. Redistributions of source code must retain the above copyright notice, this 
 *  list of conditions and the following disclaimer. 
 *  
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *  this list of conditions and the following disclaimer in the documentation and/or 
 *  other materials provided with the distribution. 
 *  
 *  This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or ITNESS FOR A 
 *  PARTICULAR PURPOSE. See the GNU General Public License for more details. A copy 
 *  of the GNU General Public License is available at: 
 *  http://www.fsf.org/licensing/licenses 
 *  
 *  All manufacturing, reproduction,use, and sales rights pertaining to this subject 
 *  matter are governed bythe license agreement. The recipient of this software 
 *  implicitly accepts the terms of the license.
 * 
 * @Description:从打包好的mod文件中解析出需要的模块 
 * @version: 
 * @Author: Zhang Sheng
 * @Date: 2019-11-20 15:26:47
 * @LastEditTime: 2019-11-20 15:30:06
 * @E-Mail: zerone216@163.com
 * @Homepage: http://www.zerone216.top
 * @Repository: https://github.com/zerone216/
 * @Support: 
 * @Others: 
 * @Modifylogs: 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <stdarg.h> /* varargs */
#include <errno.h>  /* ERANGE */
#include <ctype.h>
#include <getopt.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <linux/hdreg.h>

#define TRUE 1
#define FALSE 0

#define SECTOR_SIZE 512
#define ErrExp() printf("Assert at %s:%d,[%d]:%s\n", __FILE__, __LINE__, errno, strerror(errno))

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define XFF(a) (a & 0xFF)
#define SNIF_SIGN(p) \
    XFF(p[3]), XFF(p[2]), XFF(p[1]), XFF(p[0])

#define SUBMODNUM 32
#define MODNUM 32

#define zMalloc(type, num) (type *)malloc_type(sizeof(type), num)
#define zFree(p) free_type((void **)p)

#pragma pack(1)

typedef struct ProductId
{
    unsigned char BootMedium;    //产品载体类型
    unsigned char SupportVendor; //支持厂家品牌
    unsigned char reserved;      //预留
    unsigned char ProductType;   //具体产品类型
} St_ProductId;

typedef struct ProductVer
{
    short MajorVer;     //主版本号
    short MinorVer;     //次版本号
    short ObjectVer;    //目标版本号
    short CompileCount; //编译批次
} St_ProductVer;

typedef struct SubModAttr
{
    unsigned char module_ID[16]; //模块名
    int modoffset;               //相对模块起始位置的偏移(单位: 扇区)
    unsigned char module_status; //模块状态: 模块active = 0(默认); disable= 1;
    unsigned char reserved[11];
} St_SubModAttr;

struct EMOD_HEAD
{
    short sig;                           //0xaa55
    unsigned char module_ID[16];         //模块名
    unsigned char checksum;              //模块数据(包括模块头)校验和,数据累加等于0
    St_ProductId productid;              //产品ID
    St_ProductVer productver;            //产品版本号
    int module_size;                     //模块总大小(包括模块头) (单位: 扇区, 512字节)
    unsigned char submodnum;             //子模块数
    St_SubModAttr submodattr[SUBMODNUM]; //子模块位置信息
};

struct EMODULE_INFO
{
    unsigned char module_ID[16]; //模块名
    int mod_secoffset;           //模块(包括模块头)起始扇区相对bsmp_sec的向前//偏移的位置(单位：扇区)
    int module_size;             //模块(包括子模块头)大小,以扇区为单位
    unsigned char reserved[8];
};

struct EBSMP_INFO
{
    unsigned char signature[4];             //’EBSM’
    int structlen;                          //结构长度
    unsigned char checksum;                 // structlen累加校验和，总的校验和加起来=0；
    int total_sec;                          //在安装时预留的存放模块的扇区数，
    int used_sec;                           //模块已经使用的扇区总数;
    short module_num;                       //模块数目
    struct EMODULE_INFO Modulelist[MODNUM]; //模块列表，按模块的物理位置排序，模块可以动态地移动位置，所以所有模块的位置都必须从这个结构得到，
};

#pragma pack()

static int displayProductId(St_ProductId *productid)
{
    printf("ProductId=[%X-%X-%X-%X]\n", productid->BootMedium, productid->SupportVendor, productid->reserved, productid->ProductType);
    return 0;
}

static int displayProductVer(St_ProductVer *productver)
{
    printf("ProductVer=[%d.%d.%d.%d]\n", productver->MajorVer, productver->MinorVer, productver->ObjectVer, productver->CompileCount);
    return 0;
}

static void *malloc_type(int typesize, int num)
{
    int len = typesize * num;
    if (0 == len)
    {
        printf("malloc size is zero!\n");
        return NULL;
    }

    void *p = malloc(len);
    if (NULL == p)
    {
        ErrExp();
        return NULL;
    }

    printf("[@] malloc at [0x%p], total [%d] bytes.\n", p, len);
    memset(p, 0x00, len);
    return p;
}

static void free_type(void **p)
{
    if (*p)
    {
        printf("[#] free at [0x%p].\n", *p);
        free(*p);
        *p = NULL;
    }
}

static int openFile(const char *fileName, int mode)
{
    int fd = open(fileName, mode);
    if (fd <= 0)
    {
        ErrExp();
        return -1;
    }

    printf("open %s succeed, fd[%d].\n", fileName, fd);
    return fd;
}

static void closeFile(int *fd)
{
    if (*fd)
    {
        printf("close fd[%d] succeed.\n", *fd);
        close(*fd);
        *fd = 0;
    }
}

static int readBlockFromFile(int fd, int offset, unsigned char *buf, int len)
{
    if (lseek(fd, offset, SEEK_SET) < 0)
    {
        printf("lseek failed!\n");
        ErrExp();
        return -1;
    }

    if (read(fd, buf, len) != len)
    {
        printf("read failed!\n");
        ErrExp();
        return -1;
    }

    return 0;
}

static int writeBlockToFile(int fd, int offset, unsigned char *buf, int len)
{
    if (lseek(fd, offset, SEEK_SET) < 0)
    {
        printf("lseek failed!\n");
        ErrExp();
        return -1;
    }

    if (write(fd, buf, len) != len)
    {
        printf("write failed!\n");
        ErrExp();
        return -1;
    }

    return 0;
}

static int readSectorFromFile(int fd, int start, int num, unsigned char *buf, int len)
{
    return readBlockFromFile(fd, start * SECTOR_SIZE, buf, MIN(num * SECTOR_SIZE, len));
}

static int writeSectorToFile(int fd, int start, int num, unsigned char *buf, int len)
{
    return writeBlockToFile(fd, start * SECTOR_SIZE, buf, MIN(num * SECTOR_SIZE, len));
}

static int getFileSize(const char * filename)
{
    int fd = open(filename, O_RDWR);
    if(fd <= 0) //open failed, file not exists
    {
        printf("open %s failed!\n", filename);
        ErrExp();
        return -1;
    }

    int filesize = lseek(fd, 0, SEEK_END);
    close(fd);

    return filesize;
}

static int dumpMemToFile(unsigned char *buf, long long len, const char *filename)
{
    int fd = open(filename, O_RDWR);
    if(fd <= 0) //open failed, file not exists
    {
        fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO); //create new file
        if (fd <= 0)
        {
            printf("open %s failed!\n", filename);
            ErrExp();
            return -1;
        }
    }

    lseek(fd, 0, SEEK_END); //append write

    if (write(fd, buf, len) != len)
    {
        printf("write %s error!\n", filename);
        ErrExp();
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static int dumpFileToMem(const char *filename, unsigned char *buf, long long len)
{
    int fd = open(filename, O_RDONLY);
    if (fd <= 0)
    {
        printf("open %s failed!\n", filename);
        ErrExp();
        return -1;
    }

    if (read(fd, buf, len) != len)
    {
        printf("read %s error!\n", filename);
        ErrExp();
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int main_test(int argc, char **argv)
{
    int retval = 0;
    if (argc < 2)
    {
        retval = 1;
        goto errExit0;
    }

    int fd = openFile(argv[1], O_RDONLY);
    if (fd <= 0)
    {
        retval = 1;
        goto errExit0;
    }

    int tmpSize = 1024;
    unsigned char *tmpBuf = zMalloc(unsigned char, tmpSize);
    if (!tmpBuf)
    {
        retval = 1;
        goto errExit1;
    }

    if (readBlockFromFile(fd, 0, tmpBuf, tmpSize) != 0)
    {
        retval = 1;
        goto errExit2;
    }

    struct EBSMP_INFO *pBsmpInfo = zMalloc(struct EBSMP_INFO, 1);
    if (!pBsmpInfo)
    {
        retval = 1;
        goto errExit2;
    }

    memcpy(pBsmpInfo, tmpBuf, MIN(sizeof(struct EBSMP_INFO), tmpSize));
    printf("sizeof(struct EBSMP_INFO)=[%d]\n", sizeof(struct EBSMP_INFO));
    printf("signature=[%c%c%c%c]\n", SNIF_SIGN(pBsmpInfo->signature));
    printf("checksum=[0x%x]\n", pBsmpInfo->checksum);
    printf("module_num=[%d]\n", pBsmpInfo->module_num);

    struct EMOD_HEAD *pEmodHead = zMalloc(struct EMOD_HEAD, 1);
    if (!pEmodHead)
    {
        retval = 1;
        goto errExit3;
    }

    int i = 0;
    for (i = 0; i < pBsmpInfo->module_num; i++)
    {
        printf("-------------module[%d/%d]:ID=[%s]-------------\n", i, pBsmpInfo->module_num, pBsmpInfo->Modulelist[i].module_ID);

        if (readBlockFromFile(fd, pBsmpInfo->Modulelist[i].mod_secoffset, (unsigned char *)pEmodHead, sizeof(struct EMOD_HEAD)) != 0)
        {
            retval = 1;
            break;
        }

        int k = 0;
        for (k = 0; k < pEmodHead->submodnum; k++)
        {
            printf("==========module_ID:%s===========\n", pEmodHead->submodattr[k].module_ID);
        }
    }

errExit4:
    zFree(&pEmodHead);

errExit3:
    zFree(&pBsmpInfo);

errExit2:
    zFree(&tmpBuf);

errExit1:
    closeFile(&fd);

errExit0:
    return retval;
}

int main(int argc, char **argv)
{
    int retval = 0;
    if (argc < 2)
    {
        retval = 1;
        goto errExit0;
    }

    int filesize = getFileSize(argv[1]);
    printf("filesize=[%d]\n", filesize);
    
    int fd = openFile(argv[1], O_RDONLY);
    if (fd <= 0)
    {
        retval = 1;
        goto errExit0;
    }

    struct EMOD_HEAD *pEmodHead = zMalloc(struct EMOD_HEAD, 1);
    if (!pEmodHead)
    {
        retval = 1;
        goto errExit1;
    }

    if (readBlockFromFile(fd, 512, (unsigned char *)pEmodHead, sizeof(struct EMOD_HEAD)) != 0)
    {
        retval = 1;
        goto errExit2;
    }
    
    printf("sig=[0x%x]\n", pEmodHead->sig & 0xFFFF);
    printf("module_ID=[%s]\n", pEmodHead->module_ID);
    printf("checksum=[0x%x]\n", pEmodHead->checksum);
    displayProductId(&pEmodHead->productid);
    displayProductVer(&pEmodHead->productver);
    printf("module_size=[%d]\n", pEmodHead->module_size);
    printf("submodnum=[%d]\n", pEmodHead->submodnum);

    int k = 0;
    for (k = 0; k < pEmodHead->submodnum; k++)
    {
        printf("==========submodule[%d]ID:%s===========\n", k, pEmodHead->submodattr[k].module_ID);
        printf("modoffset=[%d]\n", pEmodHead->submodattr[k].modoffset);
        printf("module_status=[%d]\n", pEmodHead->submodattr[k].module_status);
    }

errExit2:
    zFree(&pEmodHead);

errExit1:
    closeFile(&fd);

errExit0:
    return retval;
}
