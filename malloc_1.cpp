#include <unistd.h>

void* smalloc(size_t size) {
    if(size==0||size>100000000){
        return NULL;
    }
    void* first_allocated_byte=sbrk(size); //if the break was increased the old break will be a pointer to the start of the newly allocated memory
    if(first_allocated_byte==(void*)(-1)){
        return NULL;
    }
    return first_allocated_byte;
}