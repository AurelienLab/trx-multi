/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   multistructure.h
 * Author: VertiDesk
 *
 * Created on 27 janvier 2018, 19:10
 */

#ifndef MULTISTRUCTURE_H
#define MULTISTRUCTURE_H

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

typedef struct Slot Slot;
struct Slot {
    int pid;
    int portNumber;
    int isFree;
    SOCKET sock;
};

#endif /* MULTISTRUCTURE_H */

