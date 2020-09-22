#include "CustomAllocator.h"
using namespace std;
#include <iostream>
#include <bits/stdc++.h>


/** Default constructor */
CustomAllocator::  CustomAllocator(size_t size)
{
    unsigned short buddyMinimumSize= powerOfTwo(get_closestLargerExponent(2*c_pointerSize)+6);
    if(size<buddyMinimumSize)
    {
        cout<<endl<<"The size that has been chosen is too small. Instead the number of used bytes is "
            <<buddyMinimumSize<<endl;
        size=buddyMinimumSize;
    }
    //temporary until slab allocator is implemented

    if(!(wholeBlockStart_ = (void**) malloc(size)))
    {
        cout<<"Error: Not enough free memory in the system - malloc didn't succeed. "<<endl;
        throw bad_alloc(); //if malloc does not succeed
    }
    set_wholeBlockSize(size);
    initialize();
}

/** Default destructor */
CustomAllocator:: ~CustomAllocator()
{
    free(wholeBlockStart_);
}


//
//inside functions
//


void CustomAllocator:: removeBlockFromTheFreeListIndex(size_t blockIndex)
{
//removes a block from the free list by its index

    void** blockAddress=get_blockAddress(blockIndex);
    unsigned short blockLevel=get_blockLevel(blockIndex);

    if(blockAddress==get_addressOfTheFirstFreeBlockOfLevel(blockLevel))
        // the pointer will be updated in the removeTheFirstBlock...
        removeTheFirstBlockInTheFreeListAndReturnItsIndex(blockLevel);
    else
    {
        void** block = get_blockAddress(blockIndex);
        void** previousAddress =(void**)*(block+1);
        void** nextAddress =(void**)*(block);
        if(previousAddress)
            *previousAddress = nextAddress;//next of previous = nextAddress
        if(nextAddress)
            *(nextAddress+1) = previousAddress;//previous of next= previousAddress
    }
}

unsigned short CustomAllocator ::  howManyLevelsUpwardIsTheFreeLargerBlock(unsigned short exponent)
{
    //used when split is needed (there is no free block with the requested size
    if(exponent>=wholeBlockSizeExponent_)
        return 0;

    unsigned short howManyLevelsUpward=1;
    bool weReachedBiggerAndFree=0;
    unsigned short howManyLevelsUpwardAreThere = wholeBlockSizeExponent_-exponent;

    while(!weReachedBiggerAndFree && howManyLevelsUpward<howManyLevelsUpwardAreThere)
    {
        weReachedBiggerAndFree=isThereFreeBlock(exponent+howManyLevelsUpward);
        howManyLevelsUpward+=!weReachedBiggerAndFree;
    }

    if(howManyLevelsUpward==howManyLevelsUpwardAreThere)
        return 0;

    return (howManyLevelsUpwardAreThere+1+howManyLevelsUpward) % (howManyLevelsUpwardAreThere+1);
}

void CustomAllocator:: addBlockInTheFreeList(size_t blockIndex)
{
    void** blockAddress=get_blockAddress(blockIndex);
    unsigned short blockLevel=get_blockLevel(blockIndex);
    void** addressOfCurrentFirstInTheListOfLevel=get_addressOfTheFirstFreeBlockOfLevel(blockLevel);

    if(addressOfCurrentFirstInTheListOfLevel==NULL)
        *(blockAddress)=NULL; //sets next because there was no free of this level until now
    else
    {
        *(addressOfCurrentFirstInTheListOfLevel+1)=blockAddress; //sets previous
        *blockAddress=addressOfCurrentFirstInTheListOfLevel; //sets next
    }

    *(blockAddress+1)=NULL; //sets previous
    set_thePointerToTheFirstFreeBlockOfLevel(blockLevel, blockAddress);
    //sets the pointer of the first free
    set_allocated(blockIndex,false);
    set_split(blockIndex,false);
}

size_t CustomAllocator:: removeTheFirstBlockInTheFreeListAndReturnItsIndex(unsigned short blockLevel)
{
    void** addressOfCurrentFirstInTheListOfLevel=get_addressOfTheFirstFreeBlockOfLevel(blockLevel);
    if(addressOfCurrentFirstInTheListOfLevel==NULL)
        return lastIndex_+1; // error code

    //wholeBlockSizeExponent_-blockSizeExponent is the same as blockLevel
    unsigned short exponent=wholeBlockSizeExponent_-blockLevel;
    size_t blockIndexOfFirstFreeOfLevel=get_blockIndex(addressOfCurrentFirstInTheListOfLevel,exponent);
    void ** addressOfNext = (void**)*(addressOfCurrentFirstInTheListOfLevel);
    if(addressOfNext!=NULL)//if it is NULL error will occur
        *(addressOfNext+1)=NULL;//previous of next, which will become first now is set to NULL

    set_thePointerToTheFirstFreeBlockOfLevel(blockLevel, addressOfNext);
    set_allocated(blockIndexOfFirstFreeOfLevel,true);
    return blockIndexOfFirstFreeOfLevel; //used in split function
}

//
//initializations
//

void CustomAllocator:: initialize()
{
    set_wholeBlockSizeExponent();
    unsigned short buddyMinimumSizeExponent= get_closestLargerExponent(2*c_pointerSize)+6;
    // this value is chosen in order to use less percentage of the block for its bookkeeping and
    // the smallest blocks to be at least two times bigger than the size of a pointer/machine word
    // so the size is calculated using the size of the machine word

    cout<<"Information about the constructed CustomAllocator: "<<endl;
    if(wholeBlockSizeExponent_>=buddyMinimumSizeExponent)
    {
        cout<<"Buddy Allocator is used."<<endl;
        initializeBuddy();
        pureSlab_=false;
    }
    else
    {
        cout<<"Slab Allocator is used."<<endl;
        // initializeSlab();
        pureSlab_=true;
    }
};

void CustomAllocator:: initializeBuddy()
{
    set_wholeBlockSizeAccordingToBookkeeping();
    size_t offset= (size_t)((wholeBlockSizeAccordingToBookkeeping_-wholeBlockSize_)/c_pointerSize);
    wholeBlockStartAccordingToBookkeeping_ = wholeBlockStart_ - offset;
    set_smallestAllocatableBlockSizeExponent();
    set_smallestAllocatableBlockSizeInBytes();
    set_numberOfLevels();
    set_lastIndex();

    size_t usedBytes=(size_t)(wholeBlockStart_- wholeBlockStartAccordingToBookkeeping_)*c_pointerSize;
    size_t realUsedBytes=0;// used only for cout some data
    realUsedBytes+=initialiseTheBitfieldsAndReturnTheUsedBytes();
    realUsedBytes+=set_bookkeepingOffsetAndReturnTheUsedBytes();
    initialiseThePointers();
    usedBytes+=realUsedBytes;
    size_t additionToFillTheBlock=smallestAllocatableBlockSizeInBytes_-usedBytes%smallestAllocatableBlockSizeInBytes_;
    usedBytes+=additionToFillTheBlock;
    //because - division of integers -> 7/2=3 but I've used 4 blocks
    realUsedBytes+=additionToFillTheBlock;

    //cout the info about the constructed block

    cout<<"There are " <<numberOfLevels_<<" levels."<<endl
        <<"The smallest allocatable block size is "<<(smallestAllocatableBlockSizeInBytes_)
        <<" bytes."
        <<endl<<"The used memory for the bookkeeping is "<<realUsedBytes<< " bytes of the "
        <<wholeBlockSize_<<" bytes "<<endl<<"which is "<< (wholeBlockSize_/realUsedBytes)
        <<" times smaller than the whole block size."<<endl;

    usedBytes/=smallestAllocatableBlockSizeInBytes_;
    allocateMemoryForTheBookkeeping(usedBytes);
}

void CustomAllocator:: allocateMemoryForTheBookkeeping(size_t usedLeafsNumber)
{
    set_allocated(0,true);
    set_split(0,true);
    //level by level fills up the information
    //starting by the leafs
    size_t indexOfFirstLeaf=powerOfTwo(numberOfLevels_-1)-1;

    for(size_t i=0; i<usedLeafsNumber; ++i)
        set_allocated(indexOfFirstLeaf+i,true);

    bool oddNumber=usedLeafsNumber%2;

    if(oddNumber)
        addBlockInTheFreeList(indexOfFirstLeaf+usedLeafsNumber);
    //the right buddy has not been allocated

    size_t blocksInvolvedOnThisLevel=usedLeafsNumber/2+oddNumber; //division of integers

    for(unsigned short j=1; j<numberOfLevels_-1; ++j)
    {
        size_t indexOfFirstOfThisLevel=get_indexOfFirstOfLevel(numberOfLevels_-j-1);
        for(size_t k=0; k<blocksInvolvedOnThisLevel; ++k)
        {
            set_allocated(indexOfFirstOfThisLevel+k,true);
            set_split(indexOfFirstOfThisLevel+k,true);
        }

        oddNumber=blocksInvolvedOnThisLevel%2;
        if(oddNumber)
            addBlockInTheFreeList(indexOfFirstOfThisLevel+blocksInvolvedOnThisLevel);
        //the right buddy has not been allocated

        blocksInvolvedOnThisLevel/=2;
        blocksInvolvedOnThisLevel+=oddNumber;
    }
}

void CustomAllocator:: initialiseThePointers()
{
    *(splitTableOffset_)=bookkeepingOffset_;
    //set the pointer of the first
    // element of the free list for
    // level 0 to block 0 address
    //after allocation for the bookkeeping it will be removed

    *(bookkeepingOffset_)=NULL;
    *(bookkeepingOffset_+1)=NULL; //setting next and previous of it to NULL

    for(int j=1; j<numberOfLevels_; ++j)
        *(splitTableOffset_+j)=NULL;//initialize the other pointers with NULL
}

size_t CustomAllocator:: initialiseTheBitfieldsAndReturnTheUsedBytes()
{
    //initializes the isAllocated and isSplit bits
    size_t numberOfBytesForTheFreeTable =(powerOfTwo(numberOfLevels_-3));

    set_freeTableOffset(wholeBlockStart_+numberOfBytesForTheFreeTable/c_pointerSize);
    freeTableBitfield_=(char*)wholeBlockStart_;

    set_splitTableOffset(freeTableOffset_+numberOfBytesForTheFreeTable/(2*c_pointerSize));
    splitTableBitfield_=(char*)(freeTableOffset_);
    memset((void*)freeTableBitfield_,0,numberOfBytesForTheFreeTable);
    memset((void*)splitTableBitfield_,0,numberOfBytesForTheFreeTable/2);//sets all bits to 0

    return    numberOfBytesForTheFreeTable*3/2;//for calculation of the size of the bookkeeping
}

//
//core functions
//


void* CustomAllocator::  allocate(size_t newAllocationSize)
{
    if(newAllocationSize==0)
    {
        cerr<<"Warning: Requested allocation of 0 bytes. NULL is returned."<<endl;
        return NULL;
    }
    else if(newAllocationSize<=smallestAllocatableBlockSizeInBytes_)
        return allocateExponentSize(smallestAllocatableBlockSizeExponent_);

    unsigned short closestLargerExponent = get_closestLargerExponent(newAllocationSize);
    return allocateExponentSize(closestLargerExponent);
}

void* CustomAllocator:: allocateExponentSize(unsigned short newAllocationSizeExponent)
{
    if(!isThereFreeBlock(newAllocationSizeExponent))
        return splitUntilThereIsAFreeBlockWithThisSize(newAllocationSizeExponent);
    //it calls the same function when there is a free block of that level
    else
    {
        unsigned short blockLevel=get_blockLevel(newAllocationSizeExponent);
        void** pointerToTheNewAllocatedMemory=get_addressOfTheFirstFreeBlockOfLevel(blockLevel);
        size_t blockIndex = removeTheFirstBlockInTheFreeListAndReturnItsIndex(blockLevel);
        set_allocated(blockIndex,true);
        set_split(blockIndex,false);
        memset(pointerToTheNewAllocatedMemory,0,powerOfTwo(newAllocationSizeExponent));
        //sets the bits in the newly allocated block to 0
        //cout<<"Successful allocation of "<<powerOfTwo(newAllocationSizeExponent)<<" bytes."<<endl;
        return pointerToTheNewAllocatedMemory;
    }
}


void CustomAllocator:: split(unsigned short level)
{
    size_t indexOfTheBlockBeingSplit=removeTheFirstBlockInTheFreeListAndReturnItsIndex(level);

    if(indexOfTheBlockBeingSplit<lastIndex_+1)
    {
        set_split(indexOfTheBlockBeingSplit,true);
        //remove the block that is being split from the free list and mark it as split
        set_allocated(indexOfTheBlockBeingSplit,true);
        set_split(indexOfTheBlockBeingSplit*2+1,false);//now sets info about the two new buddies
        set_split(indexOfTheBlockBeingSplit*2+2,false);
        set_allocated(indexOfTheBlockBeingSplit*2+1,false);
        set_allocated(indexOfTheBlockBeingSplit*2+2,false);
        addBlockInTheFreeList(indexOfTheBlockBeingSplit*2+2);
        //put the right buddy at the start of the list
        addBlockInTheFreeList(indexOfTheBlockBeingSplit*2+1);
        //put the left buddy at the start of the list so the right become second in the list
    }
}

void* CustomAllocator:: splitUntilThereIsAFreeBlockWithThisSize(unsigned short exponent)
{
    unsigned howManyLevelsUpward=howManyLevelsUpwardIsTheFreeLargerBlock(exponent);

    try
    {
        if(!howManyLevelsUpward)
//howManyLevelsUpwardIsTheFreeLargerBlock() returns 0 if it there is no large enough block
        {
            bad_alloc exception;
            throw exception;
        }

        for(unsigned short i=howManyLevelsUpward; i>0; --i)
            split(wholeBlockSizeExponent_- exponent-i);//split at that level

        return allocateExponentSize(exponent);
    }

    catch(bad_alloc exception)
    {
        size_t currentLargest=get_theCurrentLargestFreeBlockSize();
        cerr<<"Error: Bad Alloc - lack of memory | Allocation request cannot be done because of lack"
            <<" of memory. The largest possible allocation currently is " << currentLargest
            <<" you can deallocate some blocks in order to free some memory."<<endl;
        return NULL;
    }
}

bool CustomAllocator:: deallocate(void* pointer)
{
    if(pointer)
    {
        if(pointer < bookkeepingOffset_)
        {
            cerr<<"Warning: The deallocation had no effect | this block is used to store information"
                << "about the CustomAllocator. It cannot be deallocated before the deallocation of the"
                <<" whole block"<<endl;
            return 0;
        }
        else
        {
            size_t blockIndex = get_blockIndex((void**)pointer);
            if(blockIndex==lastIndex_+1)
            {
                cerr<<"Warning: The deallocation had no effect | attempt to free at address "
                    <<&pointer << " where is no allocated memory."<<endl;
                return 0;
            }
            else
            {
                coalesceOrAddToTheFreeList(blockIndex);
                //thought of been done in another function in order deallocation to be faster
                // but it seems not an option
                set_allocated(blockIndex,false);
                set_split(blockIndex,false);
                //          cout<<"Successful deallocation of "<<powerOfTwo(wholeBlockSizeExponent_
                //              <<- blockLevelIndex(blockIndex))<<" bytes"<<endl;
                return 1;
            }
        }
    }

    else
    {
        cerr<<"Warning: The deallocation had no effect | NULL pointer was passed. "<<endl;
        return 0;
    }
}

void CustomAllocator:: coalesceOrAddToTheFreeList(size_t blockIndex)
{
    bool isLeftBuddy=blockIndex%2==1;

    if(isLeftBuddy && !isAllocated(blockIndex+1))//checks the right buddy
        coalesce(blockIndex);

    else if(!isLeftBuddy && !isAllocated(blockIndex-1)) //checks the left buddy
        coalesce(blockIndex-1);

    else
        addBlockInTheFreeList(blockIndex);
}

void CustomAllocator:: coalesce(size_t leftBuddyIndex)
{
    removeBlockFromTheFreeListIndex(leftBuddyIndex);
    removeBlockFromTheFreeListIndex(leftBuddyIndex+1);

    set_allocated(leftBuddyIndex+1,false);
    set_allocated(leftBuddyIndex,false);
    set_split(leftBuddyIndex+1,false);
    set_split(leftBuddyIndex,false);
    set_allocated(leftBuddyIndex/2,false);
    set_split(leftBuddyIndex/2,false);
    coalesceOrAddToTheFreeList(leftBuddyIndex/2);
    //if there will be coalesce on the next level too
}
