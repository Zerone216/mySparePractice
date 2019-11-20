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
 * @Description: 
 * @version: 
 * @Author: Zhang Sheng
 * @Date: 2019-11-20 15:37:46
 * @LastEditTime: 2019-11-20 15:37:49
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
#include <time.h>
#include <pthread.h>

/* Binary semaphore */
typedef struct bsem
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int v;
} bsem;

typedef struct optdata
{
    char buf[16];
    int size;
} optdata;

/* optqueue */
typedef struct optqueue
{
    pthread_mutex_t rwmutex; /* used for optqueue r/w access */
    optdata opt[3];
    bsem *has_datas; /* flag as binary semaphore  */
    int total;
} optqueue;

/* ======================== SYNCHRONISATION ========================= */
/* Init semaphore to 1 or 0 */
static void bsem_init(bsem *bsem_p, int value)
{
    if (value < 0 || value > 1)
    {
        printf("bsem_init: Binary semaphore can take only values 1 or 0.\n");
        exit(1);
    }
    
    pthread_mutex_init(&(bsem_p->mutex), NULL);
    pthread_cond_init(&(bsem_p->cond), NULL);
    bsem_p->v = value;
}

/* Reset semaphore to 0 */
static void bsem_reset(bsem *bsem_p)
{
    bsem_init(bsem_p, 0);
}

/* Post to at least one thread */
static void bsem_post(bsem *bsem_p)
{
    pthread_mutex_lock(&bsem_p->mutex);
    bsem_p->v = 1;
    pthread_cond_signal(&bsem_p->cond);
    pthread_mutex_unlock(&bsem_p->mutex);
}

/* Post to all threads */
static void bsem_post_all(bsem *bsem_p)
{
    pthread_mutex_lock(&bsem_p->mutex);
    bsem_p->v = 1;
    pthread_cond_broadcast(&bsem_p->cond);
    pthread_mutex_unlock(&bsem_p->mutex);
}

/* Wait on semaphore until semaphore has value 0 */
static void bsem_wait(bsem *bsem_p)
{
    pthread_mutex_lock(&bsem_p->mutex);

    while (bsem_p->v != 1)
    {
        pthread_cond_wait(&bsem_p->cond, &bsem_p->mutex);
    }

    bsem_p->v = 0;
    pthread_mutex_unlock(&bsem_p->mutex);
}

optqueue *optqueueInit(int n)
{
    optqueue *obj = (optqueue *)malloc(sizeof(optqueue));
    if (!obj)
        return NULL;

    memset(obj, 0x00, sizeof(optqueue));
    obj->total = n;
    pthread_mutex_init(&(obj->rwmutex), NULL);

    obj->has_datas = (bsem *)malloc(sizeof(bsem));
    if (!obj->has_datas)
    {
        free(obj);
        return NULL;
    }

    memset(obj->has_datas, 0x00, sizeof(bsem));
    bsem_init(obj->has_datas, 0);

    return obj;
}

void optqueueDestroy(optqueue *obj)
{
    if (obj)
    {
        if (obj->has_datas)
        {
            pthread_mutex_destroy(&obj->has_datas->mutex);
            pthread_cond_destroy(&obj->has_datas->cond);
            free(obj->has_datas);
        }

        pthread_mutex_destroy(&obj->rwmutex);
        free(obj);
    }
}

void first(optqueue *obj)
{

    printf("first start ...\n");
    int i = 0;
    while (i < obj->total)
    {
        printf("read[%d]\n", i);
        usleep(1000);
        i++;
    }

    printf("first end ...\n");
    pthread_exit(0);
}

void second(optqueue *obj)
{

    printf("second start ...\n");
    int i = 0;
    while (i < obj->total)
    {
        printf("write[%d]\n", i);
        usleep(1000);
        i++;
    }

    printf("second end ...\n");
    pthread_exit(0);
}

void third(optqueue *obj)
{

    printf("third start ...\n");
    int i = 0;
    while (i < obj->total)
    {
        printf("send[%d]\n", i);
        usleep(1000);
        i++;
    }

    printf("third end ...\n");
    pthread_exit(0);
}

int main(int argc, char **argv)
{
    optqueue *obj = optqueueInit(100);
    if (!obj)
        return 1;

    pthread_t thread1, thread2, thread3;
    pthread_create(&thread1, NULL, (void *)first, (void *)obj);
    pthread_create(&thread2, NULL, (void *)second, (void *)obj);
    pthread_create(&thread3, NULL, (void *)third, (void *)obj);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);

    optqueueDestroy(obj);
    return 0;
}
