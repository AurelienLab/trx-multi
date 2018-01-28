/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   multi.h
 * Author: Verti
 *
 * Created on 27 janvier 2018, 14:26
 */
#include "multistructure.h"

#ifndef MULTI_H
#define MULTI_H

SOCKET commInitServ(int nbClient);
void commListenServ(SOCKET sock, int nb_slot, Slot* clients);
int close_all_sockets(SOCKET sock, Slot* clients, int nbSlots);

SOCKET commInitClient(const char *ipaddress);
int ask_slot(SOCKET sock, int wait);
void commListenClient(SOCKET sock);
#endif /* MULTI_H */

