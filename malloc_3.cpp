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

// add 32 blocks to the array of linked list, all should be at array[10] list
void initializeBuddyAllocator(){
 if(initialized==true){
    return;
 }
 else{
    initialized=true;
    // this for the trick of having a starting point of multilple of 128kb*32
    size_t alignment (32 * 128 * 1024);
    void* initial_block_start = sbrk(0); // get the current end of the heap
    size_t align_diff = alignment - ((size_t)initial_block_start % alignment);
    void* aligned_ptr = sbrk(align_diff);
    // and now we allocate memory for the 32 blocks
    // updated for part 3 
    // ?? here i did use sbrk twice as they mentioned but its not for the same reasons... is that alright?
    void* initial_block_start = mmap(NULL, alignment, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (initial_block_start == (void*)(-1)) {
        // error, i guess we should handle it
            return;
        }
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


//                                  part 1
// helper function for part 1 
void split_block(MallocMetadata* block, size_t requested_size) {
    size_t block_size = block->size;

    while (block_size / 2 >= requested_size + sizeof(MallocMetadata)) {
        size_t new_block_size = block_size / 2;
        // the new sec half block
        MallocMetadata* new_block = (MallocMetadata*)((char*)block + new_block_size);
        new_block->size = new_block_size;
        new_block->is_free = true;
        new_block->prev = NULL;
        new_block->next = NULL;
        // updating the first half, adding the new sec half
        block->size = new_block_size;
        freeMetadataArray.insertMetadata(new_block);
        block_size = block->size;
    }
    // first new split block is allocated
    block->is_free = false;
}

void* smalloc(size_t size) {
    if (size == 0 || size > 100000000) {
        return NULL;
    }

    size_t requested_size = size + sizeof(MallocMetadata);

    MallocMetadata* tightest = freeMetadataArray.findTightest(requested_size);
    if (tightest != NULL) {
        freeMetadataArray.removeMetadata(tightest);
        split_block(tightest, requested_size);
        // tightest->is_free = false;
        return (void*)((char*)tightest + sizeof(MallocMetadata));
        
    }

    // update for part 3 
    // larger than 128kb
    size_t sizeToAllocate = requested_size;
    void* first_allocated_byte = mmap(NULL, sizeToAllocate, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (first_allocated_byte == (void*)(-1)) {
        return NULL;
    }

    MallocMetadata* newBlockMetadata = (MallocMetadata*)first_allocated_byte;
    newBlockMetadata->size = sizeToAllocate;
    newBlockMetadata->is_free = false;
    newBlockMetadata->prev = NULL;
    newBlockMetadata->next = NULL;

    MmapedBlocks.insert(newBlockMetadata);

    return (void*)((char*)newBlockMetadata + sizeof(MallocMetadata));
}
// partner's version
/*
void* smalloc(size_t size) {
    if (size == 0 || size > 100000000) {
        return NULL;
    }

    size_t requested_size = size + sizeof(MallocMetadata);

    if (requested_size >= 128 * 1024) {
        // Allocate memory using mmap for large allocations
        void* allocated_memory = mmap(NULL, requested_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (allocated_memory == (void*)(-1)) {
            // Handle error
            return NULL;
        }

        MallocMetadata* newBlockMetadata = (MallocMetadata*)allocated_memory;
        newBlockMetadata->size = requested_size;
        newBlockMetadata->is_free = false;
        newBlockMetadata->prev = NULL;
        newBlockMetadata->next = NULL;

        return (void*)((char*)newBlockMetadata + sizeof(MallocMetadata));
    } else {
        // Allocate memory using existing blocks for smaller allocations
        MallocMetadata* tightest = freeMetadataArray.findTightest(requested_size);
        if (tightest != NULL) {
            freeMetadataArray.removeMetadata(tightest);
            split_block(tightest, requested_size);
            return (void*)((char*)tightest + sizeof(MallocMetadata));
        }
    }
    return NULL;
}
*/


void* scalloc(size_t num, size_t size){
    size_t newSize=size*num;
    void* p=smalloc(newSize);
    if(p==NULL){
        return NULL;
    }
    memset(p,0,newSize);
    return p;
}

//                                  part 2
//                                  part 2
void coalesce_buddies(MallocMetadata* metadata) {
    size_t block_size = metadata->size;
    int order = freeMetadataArray.getIndexinArrayBySize(block_size);

    // Iterate until no more merging can be done or we reach MAX_ORDER
    while (order > 0 && order < MAX_ORDER) {
        MallocMetadata* buddy = find_buddy(metadata);
        if (buddy != NULL && buddy->is_free) {
            // Coalesce with buddyyyyy, yeahhhh buddy LIGHTWEIGHT 
            MallocMetadata* merged = merge_buddies(metadata, buddy);
            freeMetadataArray.removeMetadata(metadata);
            freeMetadataArray.removeMetadata(buddy);
            freeMetadataArray.insertMetadata(merged);
            // Update metadata for the newly merged block
            metadata = merged;
            block_size *= 2;  // Double the block size after merging
            order++;
            continue;
        }
        break; 
    }
}

MallocMetadata* find_buddy(MallocMetadata* metadata) {
    size_t block_address = reinterpret_cast<size_t>(metadata);
    size_t block_size = metadata->size;
    // using the trick to find the buddy address as it says
    size_t buddy_address = block_address ^ block_size;
    // Find the corresponding free block for the buddy address
    int index = freeMetadataArray.getIndexinArrayBySize(block_size);
    MallocMetadata* buddy = freeMetadataArray.array[index].firstBlock;
    while (buddy != nullptr) {
        if (reinterpret_cast<size_t>(buddy) == buddy_address && buddy->is_free) {
            return buddy;
        }
        buddy = buddy->next;
    }
    return nullptr;
}


MallocMetadata* merge_buddies(MallocMetadata* block1, MallocMetadata* block2) {
    size_t merged_size = block1->size + block2->size;
    MallocMetadata* merged;
// checking who came first
    if (block1 < block2) {
        merged = block1;
    } else {
        merged = block2;
    }

    merged->size = merged_size;
    merged->prev = NULL;
    merged->next = NULL;

    return merged;
}


void sfree(void* p) {
    if (p == NULL) {
        return;
    }
    MallocMetadata* metadata = (MallocMetadata*)((char*)p - sizeof(MallocMetadata));
    if (metadata->is_free) {
        return;
    }
    metadata->is_free = true;
    // updated for part 3 
    if (metadata->size >= 128 * 1024) {
        // Deallocate memory using munmap for large blocks
        if (munmap(metadata, metadata->size) == -1) {
            // Handle error
            return;
        }
    } else {
        // Attempt to coalesce with buddies for smaller blocks
        freeMetadataArray.insertMetadata(metadata);
        coalesce_buddies(metadata);
    }
}


// part 3 was updates on previous implementations
// srealloc update (after part 3) 


// helper function for srealloc
MallocMetadata* tryMergeWithBuddies(MallocMetadata* metadata, size_t required_size) {
    size_t block_size = metadata->size;
    int order = freeMetadataArray.getIndexinArrayBySize(block_size);

    while (order < MAX_ORDER) {
        MallocMetadata* buddy = find_buddy(metadata);
        if (buddy != NULL && buddy->is_free) {
            MallocMetadata* merged = merge_buddies(metadata, buddy);
            freeMetadataArray.removeMetadata(metadata);
            freeMetadataArray.removeMetadata(buddy);
            freeMetadataArray.insertMetadata(merged);
            metadata = merged;
            block_size *= 2;
            order++;

            if (block_size >= required_size) {
                return merged;
            }
        } else {
            break;
        }
    }

    return NULL;
}

void* srealloc(void* oldp, size_t size) {
    if (oldp == NULL) {
        return smalloc(size);
    }

    MallocMetadata* oldMetadata = (MallocMetadata*)((char*)oldp - sizeof(MallocMetadata));

    // Case 1: If the new size is smaller than or equal to the old size, return the same pointer
    if (size <= oldMetadata->size) {
        return oldp;
    }

    // Case 2: Try to reuse the current block without merging
    void* newp = NULL;
    if (oldMetadata->size >= size) {
        split_block(oldMetadata, size);
        return oldp;
    }

    // Case 3: Check if merging with buddy blocks can obtain a large enough block
    size_t required_size = size + sizeof(MallocMetadata);
    MallocMetadata* mergedBlock = tryMergeWithBuddies(oldMetadata, required_size);
    if (mergedBlock != NULL) {
        split_block(mergedBlock, required_size);
        memmove((void*)((char*)mergedBlock + sizeof(MallocMetadata)), oldp, oldMetadata->size);
        sfree(oldp);
        return (void*)((char*)mergedBlock + sizeof(MallocMetadata));
    }

    // Case 4: Find a different block that's large enough
    newp = smalloc(size);
    if (newp == NULL) {
        return NULL;
    }
    memmove(newp, oldp, oldMetadata->size);
    sfree(oldp);
    return newp;
}

//______________________________________stats methods_________________________________________________

size_t _num_free_blocks(){
    MallocMetadata* iterator=MmapedBlocks.firstBlock;
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
    MallocMetadata* iterator=MmapedBlocks.firstBlock;
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
    MallocMetadata* iterator=MmapedBlocks.firstBlock;
    size_t counter=0;
    while (iterator!=NULL)
    {
         counter++;
        iterator=iterator->next;
    }
    return counter;
}

size_t _num_allocated_bytes(){
    MallocMetadata* iterator=MmapedBlocks.firstBlock;
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


