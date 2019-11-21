#!/usr/bin/env python
# -*- coding: UTF-8 -*-
'''
 > Declaration: 
 >  Copyright (c) 2009-2019 Zhang Sheng (Mr. Zhang) 
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
 > FilePath: \mySparePractice\cherrytree\tree2.py
 > CreateTime: 2019-11-21 09:25:31
 > ModifyTime: 2019-11-21 09:25:33
 > LastEditors: 
 > Description: 
 > version: 
 > E-Mail: zerone216@163.com
 > Homepage: http://www.zerone216.top:8080
 > Repository: https://github.com/zerone216/
 > Support: 
 > Others: 
 > Changelogs: 
'''

from turtle import *
from random import *
from math import *

def tree(n,l):
    pd()#下笔
    #阴影效果
    t = cos(radians(heading()+45))/8+0.25
    pencolor(t,t,t)
    pensize(n/3)
    forward(l)#画树枝

    if n>0:
        b = random()*15+10 #右分支偏转角度
        c = random()*15+10 #左分支偏转角度
        d = l*(random()*0.25+0.7) #下一个分支的长度
        #右转一定角度,画右分支
        right(b)
        tree(n-1,d)
        #左转一定角度，画左分支
        left(b+c)
        tree(n-1,d)
        #转回来
        right(c)
    else:
        #画叶子
        right(90)
        n=cos(radians(heading()-45))/4+0.5
        pencolor(n,n*0.8,n*0.8)
        circle(3)
        left(90)
        #添加0.3倍的飘落叶子
        if(random()>0.7):
            pu()
            #飘落
            t = heading()
            an = -40 +random()*40
            setheading(an)
            dis = int(800*random()*0.5 + 400*random()*0.3 + 200*random()*0.2)
            forward(dis)
            setheading(t)
            #画叶子
            pd()
            right(90)
            n = cos(radians(heading()-45))/4+0.5
            pencolor(n*0.5+0.5,0.4+n*0.4,0.4+n*0.4)
            circle(2)
            left(90)
            pu()
            #返回
            t=heading()
            setheading(an)
            backward(dis)
            setheading(t)
    pu()
    backward(l)#退回

bgcolor(0.5,0.5,0.5)#背景色
ht()#隐藏turtle
speed(0)#速度 1-10渐进，0 最快
tracer(0,0)
pu()#抬笔
backward(100)
left(90)#左转90度
pu()#抬笔
backward(300)#后退300
tree(12,100)#递归7层
done()
