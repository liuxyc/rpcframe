/*
 * Copyright (c) 2015-2017, Xiaoyu Liu <liuxyc at gmail dot com>
 * All rights reserved.
 */

#include "Epoll.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

Epoll::Epoll()
: m_epoll_fd(-1)
, m_max_fd(-1)
{
}

Epoll::~Epoll()
{
    close(m_epoll_fd);
}

int Epoll::Create(int max_fd)
{
    m_max_fd = max_fd;
    m_epoll_fd = epoll_create(max_fd);  

    if(fcntl(m_epoll_fd,F_SETFL, O_NONBLOCK) < 0) {
        return -1;
    }

    m_events.reset(new epoll_event[max_fd]);
    return m_epoll_fd;
}

int Epoll::Del(int fd)
{
    return epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
}

int Epoll::Add(int fd, void *data, int events)
{
    struct epoll_event ev;  
    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.ptr = data;
    return epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev);  
}

int Epoll::Mod(int fd, void *data, int events)
{
    struct epoll_event event_mod;  
    memset(&event_mod, 0, sizeof(event_mod));
    event_mod.events = events;
    event_mod.data.ptr = data;
    return epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &event_mod);
}

int Epoll::Wait(int timeout)
{
    return epoll_wait(m_epoll_fd, m_events.get(), m_max_fd, timeout);  
}

void *Epoll::getData(int index) 
{
    if(index >= m_max_fd) {
        return nullptr;
    }
    return m_events[index].data.ptr;
}

int Epoll::getEvent(int index) 
{
    if(index >= m_max_fd) {
        return -1;
    }
    return m_events[index].events;
}
