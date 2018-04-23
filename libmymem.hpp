#define pragma once

#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <mutex>
using namespace std;

/* Struct to store the member variables required for a bucket
objectSize denotes the size of data objects the bucket will store
firstSlab will point to firstSlab of this bucket
a mutex lock is provided to ensure only one thread works over a bucket at any time*/
struct bucket
{
    int objectSize;
    struct slab* firstSlab;
    mutex mtx;
};

/* Array to store our buckets of different sizes from 4B to 8192B*/
struct bucket buckets[12] = {
    {4 , NULL},
    {8 , NULL},
    {16 , NULL},
    {32 , NULL},
    {64 , NULL},
    {128 , NULL},
    {256 , NULL},
    {512 , NULL},
    {1024 , NULL},
    {2048 , NULL},
    {4096 , NULL},
    {8192 , NULL}
};

/* structure of data object, contains pointer to the slab holding this object,
 and the data to write*/
struct object
{
    struct slab* slabptr;
    void* data;
};


/*structure of slab
totalObj denotes total no of objects present in slab
freeObj denotes total unallocated objects present at any instant in the slab
bitmap to increases the time-efficiency to find an unallocated object to allocate it.
bucketPtr to point to the bucket which the slab is a part of
nextSlab denotes pointer to our next slab
offset denotes the size of header part present in each slab*/
struct slab
{
    int totalObj;
    int freeObj;
    bitset<16000> bitmap;
    struct bucket* bucketPtr;
    struct slab* nextSlab;
    void* offset;
};

// function to allocate memory of the input size
void* mymalloc(unsigned size);
// function to free the memory pointed by the input pointer
void myfree(void *t_ptr);