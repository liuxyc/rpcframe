/*
 * Copyright (c) 2015-2017, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#pragma once
#include <sys/epoll.h>  
#include <memory>

class Epoll 
{
public:
    Epoll();
    ~Epoll();
    int Create(int max_fd);
    int Add(int fd, void *data, int events);
    int Del(int fd);
    int Mod(int fd, void *data, int events);
    int Wait(int timeout);
    void *getData(int index);
    int getEvent(int index);

private:
    int m_epoll_fd;
    int m_max_fd;
    std::unique_ptr<struct epoll_event[]> m_events;
};
