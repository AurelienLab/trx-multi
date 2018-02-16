/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   admin.h
 * Author: VertiDesk
 *
 * Created on 7 février 2018, 01:49
 */

#ifndef ADMIN_H
#define ADMIN_H

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;


typedef struct Admin Admin;
struct Admin {
    SOCKET socket;
    char ip;
};

SOCKET admin_init_socket();
int admin_manage(SOCKET adminSock, Client* clients, Client* waitlist[], Slot* slots, int nbSlots);
#endif /* ADMIN_H */
