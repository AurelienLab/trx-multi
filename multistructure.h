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
#include <opus/opus.h>

#ifndef MULTISTRUCTURE_H
#define MULTISTRUCTURE_H

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;


typedef struct Options Options;
struct Options {
    OpusDecoder *decoder;
    
    const char *device;
    const char *addr;
    const char *pid;
    
    unsigned int buffer;
    unsigned int rate;
    unsigned int jitter;
    unsigned int channels;
    unsigned int port;
};

typedef struct Server Server;
struct Server {
    pid_t pid;
    time_t start_time;
};
typedef struct Client Client;
struct Client {
    char ip[16];
    char name[100];
    time_t connex_time;
    int rate;
    SOCKET sock;
};

typedef struct Slot Slot;
struct Slot {
    pid_t pid;
    Client* client;
    Options param;
    time_t start_time;
};
#endif /* MULTISTRUCTURE_H */

