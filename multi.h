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

#ifndef MULTI_H
#define MULTI_H

typedef struct Slot Slot;
struct Slot {
    int pid;
    int portNumber;
    int isFree;
};

#endif /* MULTI_H */

