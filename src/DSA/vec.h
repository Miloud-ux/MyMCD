#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Vector iVector;

iVector *vectorCreate(size_t size);
void vectorPrint(iVector *self);
void vectorPush(iVector *self, int data);
void vectorPop(iVector *self);
void vectorFree(iVector *self);
int vectorGet(iVector *self, size_t index);
