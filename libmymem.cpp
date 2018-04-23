#include "libmymem.hpp"
using namespace std;
#define slabSize 65536         // The fixed slab size = 64KB

/* This function creates a new slab of size equals to input parameter "size"
and returns a slab pointer pointing to that memory location
*/
struct slab* slabCreator(unsigned size, int indexBucket)
{
    int fd;
    if ((fd = open("./1mfile", O_RDWR)) < 0) {
        perror("open");
        exit(1);
    }
    // pointer to points to the memory allocated
    void* ptr1; 

    /*mmap () creates a new mapping in the virtual address space of the calling process
    the specified size is equal to the size of slab = 64KB*/
    if ((ptr1 = mmap(NULL, slabSize, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // A pointer to traverse calculate the offset i.e. size of slabHeader
    char* ptrTraverse = (char*)ptr1;
    struct slab* ptr = (struct slab*)ptr1;

    /* The size of our slabHeader,includes the memory in the slab apart from the 
    memory allocated to the data objects*/
    int sizeheader = sizeof(ptr->totalObj) + sizeof(ptr->freeObj) + sizeof(ptr->bitmap) + sizeof(ptr->bucketPtr) + sizeof(ptr->nextSlab) 
                        + sizeof(ptr->offset);

    // max no. of objects that the slab can hold
    ptr -> totalObj = ((slabSize-sizeheader)/(sizeof(struct slab*)+size));

    ptrTraverse += sizeof(ptr->totalObj);
    ptr -> freeObj = ptr -> totalObj;
    ptrTraverse += sizeof(ptr->freeObj);

    ptr -> bucketPtr = &buckets[indexBucket];

    ptrTraverse += sizeof(ptr->bitmap);
    ptrTraverse += sizeof(ptr->bucketPtr);
    ptrTraverse += sizeof(ptr->nextSlab) + sizeof(ptr->offset);

    ptr -> nextSlab = NULL;

    ptr-> offset = ptrTraverse;

    // returning the pointer to the new allocated slab
    return ptr;
}

/*This function allocates the data-object of the input size.
It calls slabCreator() to create a new slab if we are allocating the firstSlab of a particular size
or all the slabs are fully occupied, otherwise it finds the first unallocated object of any slab
traversing from the firstSlab by checking the bitmap of each slab, and allocates the object to it.*/
void* mymalloc(unsigned size)
{
    /* the bucket index to which the object will be a part of,
    ceil is used for the objects whose size are not powers of 2*/
    int index = ceil(log2(size)) - 2;               
    int check = 1;
    buckets[index].mtx.lock();
    // if allocating the firstSlab in this bucket
    if(buckets[index].firstSlab == NULL)
    {
        buckets[index].firstSlab = slabCreator(size,index);
    }

    // current to hold the slab in which we will allocate our data-object
    struct slab* current = buckets[index].firstSlab;
    char* allocationPoint;
    do
    {
        int i = 0;

        /*finding if any data-object is free by checking its bitmap,
        if no data-object is free we jump to the next slab*/
        while(current -> bitmap[i] == 1)
            i++;

        // all data-objects are occupied
        if(i == current -> totalObj)
        {
            if(current -> nextSlab == NULL)
            {
                current -> nextSlab = slabCreator(size,index);
            }
                current = current -> nextSlab;
        }
        else
        {
            // assinging the data-object to this slab, therefore changing its bitmap
            current -> bitmap[i] = 1;
            current -> freeObj--;
            check = 0;
            // temp_addition is used as arithmetic operations on void* leads to warnings
            char* temp_addition = (char*)current->offset;
            temp_addition += i *(sizeof(struct slab*)+size);
            allocationPoint = temp_addition;
            struct object* newObj;
            newObj = (struct object*) allocationPoint;
           // newObj->data = 1;
            newObj->slabptr = current;
        }

    }while(check == 1);
    
    // the pointer where the user can write its content i.e user data
    buckets[index].mtx.unlock();
    return allocationPoint + sizeof(struct slab*);
}


/* frees the memory pointed by t_ptr to avoid memory-leaks
If the slab contains other data-objects also, we can remove our data-object and change the bitmap
else if the slab becomes empty after deleting this data-object, we need to unmap our slab
For unmapping the slab we use the bucketPtr and traverse through the slabs and remove our slab
(similiar to deleting an element from the linked-list) 
*/
void myfree(void *t_ptr)
{
    //t_came denotes the object we want to deallocate
    struct object* t_came = (struct object*) ((char*)t_ptr - sizeof(struct slab*));
    int index = (log2(t_came->slabptr->bucketPtr->objectSize)) - 2;
    buckets[index].mtx.lock();
    long int x = (char*)((char*)t_ptr-sizeof(struct slab*))-(char*)((t_came->slabptr)->offset);
    int noOfObjectsOffset = x/(t_came->slabptr->bucketPtr->objectSize + sizeof(struct slab*));
    t_came->slabptr->bitmap[noOfObjectsOffset] = 0;
    //t_came->data = -1;
    t_came->slabptr->freeObj++;
    if(t_came->slabptr->freeObj == t_came->slabptr->totalObj)
    {
        // we have to unmap the complete slab
        struct bucket* tempBucket = t_came->slabptr->bucketPtr;
        struct slab* tempSlab = tempBucket->firstSlab;
        if(t_came->slabptr == tempSlab)
        {
            tempBucket->firstSlab = t_came->slabptr->nextSlab;
            munmap((void*)(t_came->slabptr) ,slabSize);
            buckets[index].mtx.unlock();
            return;
        }
        // traversing through the slab until the tempSlab->nextSlab points to the slab to be unmapped
        while(tempSlab->nextSlab != t_came->slabptr)
            tempSlab = tempSlab->nextSlab;

        tempSlab->nextSlab = t_came->slabptr->nextSlab;

        // unmaps the slab
        munmap((void*)(t_came->slabptr) ,slabSize);

    }
    buckets[index].mtx.unlock();
    return;
}
