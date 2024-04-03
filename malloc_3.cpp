#include <unistd.h>
#include <cstring> 
#include <sys/mman.h>
#define MAX_ORDER 10


struct MallocMetadata {
 size_t size;
 bool is_free;
MallocMetadata* next;
MallocMetadata* prev;
};

//________________________________________freeBlocksList_________________________________
class freeBlocksList{
     
    public:
    MallocMetadata* firstBlock;
    MallocMetadata* lastBlock;
    freeBlocksList():firstBlock(NULL),lastBlock(NULL){};
    void insert(MallocMetadata* metadata);
    void remove(MallocMetadata* metadata);

};


void freeBlocksList::insert(MallocMetadata *metadata) //when calling ,the medata must be with prev and next sited to NULL
{
    if(firstBlock==NULL){
        firstBlock=metadata;//check
        lastBlock=metadata;//check
    }else{
        MallocMetadata* current=firstBlock;
        while(current!=NULL && current<metadata){
            current=current->next;
        }
        if(current==NULL){//insert at the end of the list
            lastBlock->next=metadata;
            metadata->prev=lastBlock;
            lastBlock=metadata;
        }else{//insert before the current
            if(current==firstBlock){
                metadata->next=firstBlock;
                firstBlock->prev=metadata;
                firstBlock=metadata;
            }else{
                metadata->next=current;
                metadata->prev=current->prev;
                current->prev->next=metadata;
                current->prev=metadata;
            }
            
        }
    }
}

void freeBlocksList::remove(MallocMetadata *metadata)
{
    //if it is the first block
    if(metadata==firstBlock){
        firstBlock=firstBlock->next;
        if(firstBlock!=NULL)
        firstBlock->prev=NULL;
        else
        lastBlock=NULL;
        metadata->next=NULL;
        metadata->prev=NULL;
        return;
    }

    //if it is the last block
    if(metadata==lastBlock){
        lastBlock=lastBlock->prev;
        if(lastBlock!=NULL)
        lastBlock->next=NULL;
        else
        firstBlock=NULL;
        metadata->next=NULL;
        metadata->prev=NULL;
        return;
    }

    //if it is in the middle
    metadata->prev->next=metadata->next;
    metadata->next->prev=metadata->prev;
        metadata->next=NULL;
        metadata->prev=NULL;

}



//___________________________________MetadataArray_________________________________

class MetadataArray{
    public:
    freeBlocksList array[MAX_ORDER];
    MetadataArray();
    int getIndexinArrayBySize(size_t blockSize);
    void insertMetadata(MallocMetadata* metadata);
    void removeMetadata(MallocMetadata* metadata);
    MallocMetadata* findMinimal(int i);//find the metadata with minimal size that is equal or bigger than thr sizes in cell i 
    MallocMetadata* findTightest(size_t size);

};
MetadataArray::MetadataArray()
{
        for (int i = 0; i <= MAX_ORDER; ++i) {
            array[i] =  freeBlocksList();
        }
}

int MetadataArray::getIndexinArrayBySize(size_t blockSize)
{
    int index = 0;
    // Divide the block size by 128 until it becomes 1
   
    while (blockSize > 128) {
        blockSize /= 2;
        index++;
    }
    return index;
}

void MetadataArray::insertMetadata(MallocMetadata *metadata)
{
    int index=getIndexinArrayBySize(metadata->size);
    array[index].insert(metadata);

}

void MetadataArray::removeMetadata(MallocMetadata *metadata)
{
    int index=getIndexinArrayBySize(metadata->size);
    array[index].remove(metadata);
}


MallocMetadata *MetadataArray::findMinimal(int i)
{
    for(;i<=MAX_ORDER;i++){
        if(array[i].firstBlock!=NULL){
            return array[i].firstBlock;
        }
    }
    return NULL;
}

MallocMetadata *MetadataArray::findTightest(size_t size)
{
    int i=getIndexinArrayBySize(size);
    return findMinimal(i);
}
//____________________________________________________________________________________________
MetadataArray freeMetadataArray=MetadataArray();
freeBlocksList MmapedBlocks=freeBlocksList();//it will have al the mmaped not just the free????
bool initialized=false;

void initializeBuddyAllocator(){
 if(initialized==true){
    return;
 }
 else{
    initialized=true;
    sbrk(128 * 32 * 1024);
    void* initial_block_start=sbrk(128*32*1024);
    for (int i = 0; i < 32; ++i) {
        //starting address of the current block
        void* current_block_address=(void*)((char*)initial_block_start + i * (128 * 1024));

        MallocMetadata* newBlockMetadata = (MallocMetadata*)current_block_address;
        newBlockMetadata->size=128*1024;
        newBlockMetadata->is_free=true;
        newBlockMetadata->prev=NULL;
        newBlockMetadata->next=NULL;
        freeMetadataArray.insertMetadata(newBlockMetadata);
    }    
 }
}


void* smalloc(size_t size){


    if(size==0||size>100000000){
        return NULL;
    }
    
    MallocMetadata* tightest=freeMetadataArray.findTightest(size);
    if(tightest!=NULL){
        freeMetadataArray.removeMetadata(tightest);
        tightest->is_free=false;
        return  (void*)((char*)tightest+sizeof(MallocMetadata));
    }
    //if reached here so we need to allocate new block :(
        //not completed yet
    size_t sizeToAllocate=size+sizeof(MallocMetadata);
    void* first_allocated_byte=mmap(NULL,sizeToAllocate, PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
    if(first_allocated_byte==(void*)(-1)){
        return NULL;
    }

    MallocMetadata* newBlockMetadata = (MallocMetadata*)first_allocated_byte;
    newBlockMetadata->size=sizeToAllocate;
    newBlockMetadata->is_free=false;
    newBlockMetadata->prev=NULL;
    newBlockMetadata->next=NULL;




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


