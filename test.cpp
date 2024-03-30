

#include <unistd.h>
#include <cstring> 

struct MallocMetadata {
 size_t size;
 bool is_free;
MallocMetadata* next;
MallocMetadata* prev;
};

class BlocksList{
     
    public:
    MallocMetadata* firstBlock;
    MallocMetadata* lastBlock;
    BlocksList():firstBlock(NULL),lastBlock(NULL){};
    
    size_t numFreeBlocks();
    size_t numFreeBytes();
    size_t numAllocatedBlocks();
    size_t numAllocatedBytes();
};


//____________________________________________________________________________________________
BlocksList blocksList= BlocksList();


void* smalloc(size_t size){
    if(size==0||size>100000000){
        return NULL;
    }
    
    MallocMetadata* iterator=blocksList.firstBlock;
    while(iterator!=NULL){
        if(iterator->is_free && iterator->size >=size){
            iterator->is_free=false;
            return (void*)((char*)iterator+sizeof(MallocMetadata));
        }
        iterator=iterator->next;
    }
    //if reached here so we need to allocate new block :(
    size_t sizeToAllocate=size+sizeof(MallocMetadata);
    void* first_allocated_byte=sbrk(sizeToAllocate);
    if(first_allocated_byte==(void*)(-1)){
        return NULL;
    }

    MallocMetadata* newBlockMetadata = (MallocMetadata*)first_allocated_byte;
    newBlockMetadata->size=size;
    newBlockMetadata->is_free=false;
    newBlockMetadata->prev=NULL;
    newBlockMetadata->next=NULL;
        //INSERTING THE NEW BLOCK IN THE LIST 
            if (blocksList.firstBlock==NULL) {
             blocksList.firstBlock=newBlockMetadata;
            } else {
             blocksList.lastBlock->next=newBlockMetadata;
             newBlockMetadata->prev= blocksList.lastBlock;
            }
        blocksList.lastBlock = newBlockMetadata;
    return (void*)((char*)newBlockMetadata + sizeof(MallocMetadata));//check ,skipp the metadata 


}

void* scalloc(size_t num, size_t size){
    size_t newSize=size*num;
    void* p=smalloc(newSize);
    if(p==NULL){
        return NULL;
    }
    memset(p,0,newSize);
    return p;
}

void sfree(void* p){
    MallocMetadata* Pmetadata = (MallocMetadata*)((char*)p - sizeof(MallocMetadata));
    if (p==NULL||Pmetadata->is_free){
        return;
    }
    Pmetadata->is_free=true;
}

void* srealloc(void* oldp, size_t size){
    if(oldp==NULL){
        return smalloc(size);
    }
     MallocMetadata* oldPmetadata = (MallocMetadata*)((char*)oldp - sizeof(MallocMetadata));
     if(size<=oldPmetadata->size){
        return oldp;
     }
     void* newP=smalloc(size);
     if(newP==NULL){
        return NULL;
     }
     memmove(newP,oldp,oldPmetadata->size);
     sfree(oldp);
     return newP;

}
//______________________________________stats methods_________________________________________________
size_t _num_free_blocks(){
    MallocMetadata* iterator=blocksList.firstBlock;
    size_t counter=0;
    while (iterator!=NULL)
    {
        if(iterator->is_free){
            counter++;
        }
        iterator=iterator->next;
    }
    return counter;
    
}

size_t _num_free_bytes(){
    MallocMetadata* iterator=blocksList.firstBlock;
    size_t counter=0;
    while (iterator!=NULL)
    {
        if(iterator->is_free){
            counter+=iterator->size;
        }
        iterator=iterator->next;
    }
    return counter;
}

size_t _num_allocated_blocks(){
    MallocMetadata* iterator=blocksList.firstBlock;
    size_t counter=0;
    while (iterator!=NULL)
    {
         counter++;
        iterator=iterator->next;
    }
    return counter;
}

size_t _num_allocated_bytes(){
    MallocMetadata* iterator=blocksList.firstBlock;
    size_t counter=0;
    while (iterator!=NULL)
    {
        counter+=iterator->size;   
        iterator=iterator->next;
    }
    return counter;  
}

size_t _num_meta_data_bytes(){
    return sizeof(MallocMetadata) * _num_allocated_blocks();
}

size_t _size_meta_data(){
    return sizeof(MallocMetadata);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <cstring> // For memset
// Include the header file where your memory allocation functions are declared
int main() {
    // Test smalloc
    std::cout << "Testing smalloc..." << std::endl;
    void* ptr1 = smalloc(100);
    if (ptr1 != nullptr) {
        std::cout << "Memory allocated successfully!" << std::endl;
    } else {
        std::cout << "Memory allocation failed!" << std::endl;
    }

    // Test scalloc
    std::cout << "\nTesting scalloc..." << std::endl;
    void* ptr2 = scalloc(10, sizeof(int));
    if (ptr2 != nullptr) {
        std::cout << "Memory allocated and initialized successfully!" << std::endl;
    } else {
        std::cout << "Memory allocation failed!" << std::endl;
    }

    // Test srealloc
    std::cout << "\nTesting srealloc..." << std::endl;
    void* ptr3 = srealloc(ptr1, 200);
    if (ptr3 != nullptr) {
        std::cout << "Memory reallocated successfully!" << std::endl;
    } else {
        std::cout << "Memory reallocation failed!" << std::endl;
    }

    // Test sfree
    std::cout << "\nTesting sfree..." << std::endl;
    sfree(ptr2);
    std::cout << "Memory freed successfully!" << std::endl;


    //smalloc after sfree
    std::cout << "Testing smalloc..." << std::endl;
    void* ptr4 = smalloc(30);
    if (ptr4 != nullptr) {
        std::cout << "Memory allocated successfully!" << std::endl;
    } else {
        std::cout << "Memory allocation failed!" << std::endl;
    }

    // Print statistics
    std::cout << "\nMemory Statistics:" << std::endl;
    std::cout << "Number of free blocks: " << _num_free_blocks() << std::endl;
    std::cout << "Number of free bytes: " << _num_free_bytes() << std::endl;
    std::cout << "Number of allocated blocks: " << _num_allocated_blocks() << std::endl;
    std::cout << "Number of allocated bytes: " << _num_allocated_bytes() << std::endl;
    std::cout << "Number of metadata bytes: " << _num_meta_data_bytes() << std::endl;
    std::cout << "Size of metadata: " << _size_meta_data() << std::endl;

    return 0;
}
