#ifndef CUSTOMALLOCATORH
#define CUSTOMALLOCATORH
#include <iostream>
using namespace std;

const size_t c_pointerSize=sizeof(void*);

class CustomAllocator
{
    /**Created by
    * \author Georgi Spasov
    * e-mail : gmspasov@uni-sofia.bg | gecta13@gmail.com
    * on 04.09.2020 20:35 as System Programming Seminar project.
    *Custom allocator using Buddy Algorithm for big allocations,
    *for small allocations it is prepared for the implementation of
    *slab allocator algorithm BUT IT HAS NOT BEEN IMPLEMENTED yet.
    *Buddy memory allocation : https://en.wikipedia.org/wiki/BuddyMemoryAllocation
    *Slab memory allocation : https://en.wikipedia.org/wiki/SlabAllocation
    *Briefly this implementation of buddy allocator is following the classical idea.
    *It operates with a block allocated at first with malloc()
    *and when request of certain size of memory comes the block is divided (split) in
    *power of two sized blocks with the suitable size (if there is enough free memory) and
    *then pointer to that certain free block is returned (for 595 bytes it would be block with 1024 bytes).
    *There is limit of how small block can be to be allocated and there is bookkeeping in which is written
    *which are the free blocks or those who have been split (a block of 2048 can be divided in two of 1024
    *in order to avoid losses. Deallocation also is provided. The information of the bookkeeping is written
    *inside the block (at the beginning), I decided to choose the limit of how small allocatable block to have
    *with the mind of that the information of the bookkeeping to fit in no more than 2 block with minimal size.
    *If the size of the whole block is not power of two - the bookkeeping works with illusion, for it the size
    *always a power of two (the difference between the two sizes is said to be constantly allocated). When a
    *block is split the left "buddy" is allocated and the right is put in the corresponding list of free blocks
    *of that level (with that size). Blocks are identified by indexes.In every block is written its next and
    * PREVIOUS free block in free list. Previous is needed when block is removed.
    * For more information you can use:
    *the Internet, C++ conferences and the video provided by my university professors :
    *https://www.youtube.com/watch?v=xXvyn6Oz7gI (in Bulgarian).I've used also information from
    *en.cppreference.com, www.geeksforgeeks.org, www.quora.com and stackoverflow.com.
    */

public:
    /** Default constructor */
    CustomAllocator(size_t);
    /** Default destructor */
    virtual ~CustomAllocator();


    //
    //getters
    //


    /** Access wholeBlockStartAccordingToBookkeeping
    * \return The current value of wholeBlockStartAccordingToBookkeeping
    */
    void** wholeBlockStartAccordingToBookkeeping() const
    {
        return wholeBlockStartAccordingToBookkeeping_;
    }

    /** Access wholeBlockSizeExponent_ - the closest larger exponent of two that is bigger than wholeBlockSize
    * \return The value of wholeBlockSizeExponent_
    */
    unsigned short wholeBlockSizeExponent() const
    {
        return wholeBlockSizeExponent_;
    }

    /** Access bookkeepingOffset - pointer to the end of the bookkeeping
     * \return The current value of bookkeepingOffset
     */
    void** bookkeepingOffset() const
    {
        return bookkeepingOffset_;
    }

    /** Access freeTableOffset - pointer to the end of the bits in which is written isAllocated
     * \return The current value of freeTableOffset
     */
    void** freeTableOffset() const
    {
        return freeTableOffset_;
    }

    /** Access splitTableOffset - pointer to the end of the bits in which is written isSplit
    * \return The current value of splitTableOffset
    */
    void** splitTableOffset() const
    {
        return splitTableOffset_;
    }

    /** Access lastIndex
    * \return The value of lastIndex
    */
    size_t lastIndex() const
    {
        return lastIndex_;
    }

    /** Access smallestAllocatableBlockSizeExponent
    * \return The value of smallestAllocatableBlockSizeExponent
    */
    unsigned short smallestAllocatableBlockSizeexponent() const
    {
        return smallestAllocatableBlockSizeExponent_;
    }

    /** Access smallestAllocatableBlockSizeInBytes
    * \return The value of smallestAllocatableBlockSizeInBytes
    */
    size_t smallestAllocatableBlockSizeInBytes() const
    {
        return smallestAllocatableBlockSizeInBytes_;
    }

    /** Access pointer to the first in the free list of level blockLevel
     *\param unsigned short blockLevel
     * \return pointer to the first in the free list of level blockLevel
     */
    void** get_addressOfTheFirstFreeBlockOfLevel(unsigned short blockLevel) const
    {
        return (void**)*(splitTableOffset_ + blockLevel);
    }

    /** Access to the level of block by its size (the size in the form of exponent of two)
    *\param unsigned short exponent
    * \return The level of the block
    */
    unsigned short get_blockLevel(unsigned short exponent) const
    {
        return wholeBlockSizeExponent_ - exponent;
    }

    /** Access to the level of block by its index
    *\param size_t blockIndex
    * \return The level of the block
    */
    unsigned short get_blockLevel(size_t blockIndex) const
    {
        return get_closestLargerExponent(blockIndex+2)-1;
    }

    /** Access to the index of a block by its size and a pointer to it
    *\param void** blockAddress
    *\param unsigned short exponent
    * \return The index of the block
    */
    size_t get_indexOfFirstOfLevel(void** blockAddress, unsigned short exponent) const
    {
        if(blockAddress==NULL)
        {
            return lastIndex_+1;
        }
        return ((get_indexOfFirstOfLevel(wholeBlockSizeExponent_-exponent)) + (((size_t)(blockAddress-wholeBlockStartAccordingToBookkeeping_))*c_pointerSize)/(powerOfTwo(exponent)));
    }

    /** Access to the index of the first block of level level
    *\param unsigned short level
    * \return The index of the block
    */
    size_t get_indexOfFirstOfLevel(unsigned short level) const
    {
        return powerOfTwo(level)-1;
    }

    /** Access to the index of an already allocated block by a pointer to it
    *\param void** pointer
    * \return The index of the block
    */
    size_t get_indexOfFirstOfLevel(void** pointer) const
    {
        size_t indexOfTheLeafOfTheBranch=((c_pointerSize*((size_t)(pointer-wholeBlockStartAccordingToBookkeeping_))/(smallestAllocatableBlockSizeInBytes_))+powerOfTwo(numberOfLevels_-1)-1);
        size_t i=indexOfTheLeafOfTheBranch;
        // Goes to the leaf that corresponds to this address and find
        // the largest allocated block that contains this memory.
        // The search is done upward. Parent index is two times smaller.

        bool odd_i=0;

        while(!isAllocated(i)&&!isSplit(i)&&i!=0) // until an allocated or a split block is found
        {
            odd_i=i%2;
            i-=!odd_i;
            i/=2;
        }

        if(isSplit(i) && i!=indexOfTheLeafOfTheBranch) // *
        {
            return lastIndex_+1;
        }
        // * : If the parent block has been split and the other
        // buddy has been allocated, warning with information is presented.

        return i;
    }

    /** Access to the address of a block by its index
    * @param size_t blockIndex
    * \return The address of the block
    */
    void** get_blockAddress(size_t blockIndex) const
    {
        unsigned short blockLevel = get_blockLevel(blockIndex);
        size_t startingBlockIndexOfTheLevel=get_indexOfFirstOfLevel(blockLevel);
        return wholeBlockStartAccordingToBookkeeping_ + (blockIndex-startingBlockIndexOfTheLevel)*powerOfTwo(wholeBlockSizeExponent_-blockLevel)/c_pointerSize;
    }

    /** Access to the size of a block by its index
    * @param size_t blockIndex
    * \return The size of the block
    */
    size_t get_size (size_t blockIndex) const
    {
        return powerOfTwo(wholeBlockSizeExponent_ - get_blockLevel(blockIndex));
    }

    /** Access of the isAllocated bit of a block by the index of the block
    * @param size_t blockIndex
    * \return isAllocated
    */
    bool isAllocated(size_t blockIndex) const
    {
        if(blockIndex>lastIndex_)
            return 0;
        size_t byteNumber=blockIndex/8; //in which byte in the free table bitfield is the block
        char byte = *(freeTableBitfield_+byteNumber); // the content of that byte
        unsigned short blockIndexInByte=blockIndex%8; // which bit in the byte corresponds to the block
        return ((byte >> blockIndexInByte) & 1U); //shifts the bit to be least significant and compare it with 00000001
    }

    /** Access of the isSplit bit of a block by the index of the block
    * @param size_t blockIndex
    * \return isSplit
    */
    bool isSplit(size_t blockIndex) const
    {
        if(blockIndex>lastIndex_/2)
            return 0;
        size_t byteNumber=blockIndex/8;//in which byte in the split table bitfield is the block
        char byte = *(splitTableBitfield_+byteNumber);// the content of that byte
        unsigned short blockIndexInByte=blockIndex%8;//which bit in the byte corresponds to the block
        return ((byte >> blockIndexInByte) & 1U);//shifts the bit to be least significant and compare it with 00000001
    }

    /** Check if there is a free block with that size (size in the form of exponent of two)
    * @param unsigned short exponent
    */
    bool isThereFreeBlock(unsigned short exponent) const
    {
        return (*(splitTableOffset_+(wholeBlockSizeExponent_- exponent))!=NULL);//if the pointer of the first free of the level is not null
        //wholeBlockSizeExponent_-exponent = blockLevel
    }

    /** Check if the block is first in its level (block presented by its index)
    * @param size_t blockIndex
    */
    bool isFirstInTheLevel(size_t blockIndex) const
    {
        return blockIndex+1==get_closestLargerPowerOfTwo(blockIndex);
    }

    /** Informative function. Returns the current largest size of a free block that is on
    *lower level than block level that has been already checked
    *\param unsigned blockLevel=0
    *\return the size of the largest available block currently.
    */
    size_t get_theCurrentLargestFreeBlockSize(unsigned short blockLevel=0) const
    {
        for(unsigned short i = blockLevel+1; i<numberOfLevels_; ++i)
        {
            if(isThereFreeBlock(wholeBlockSizeExponent_ - i))
            {
                return powerOfTwo(wholeBlockSizeExponent_-i);
            }
        }
        return 0;
    }

    /** Gets power of 2 with exponent exponent
    * @param unsigned short exponent
    *\return size_t power of 2 with exponent exponent
    */
    size_t powerOfTwo(unsigned short exponent) const
    {
        size_t power=1;
        power=power<<exponent;
        return power;
    }

    /** Gets the closest larger power of 2 than number
    * @param size_t number
    *\return size_t closest larger power of 2
    */
    size_t get_closestLargerPowerOfTwo(size_t number) const
    {
        size_t power=1;
        while(power < number)
        {
            power=power<<1;
        }
        return power;
    }

    /** Gets the closest larger exponent of two than number
    * @param size_t number
    *\return unsigned short closest larger exponent of two
    */
    unsigned short get_closestLargerExponent(size_t newAllocationSize) const
    {
        unsigned short exponent=0;
        size_t power=1;
        while(power<newAllocationSize)
        {
            power=power<<1;
            ++exponent;
        }
        return exponent;
    }


private:


    //
    //setters
    //

    /** set of wholeBlockSize
    * \param size_t size - the size of the whole block
    */
    void set_wholeBlockSize(size_t size)
    {
        wholeBlockSize_=size;
    }

    /** set of wholeBlockSizeExponent_ - calculation done inside
    */
    void set_wholeBlockSizeExponent()
    {
        wholeBlockSizeExponent_=get_closestLargerExponent(wholeBlockSize_);
    }

    /** set of bookkeepingOffset - calculation done inside
    *\return the number of bytes used - sent to allocateMemoryForTheBookkeeping
    */
    size_t set_bookkeepingOffsetAndReturnTheUsedBytes()
    {
        bookkeepingOffset_=splitTableOffset_+numberOfLevels_;
        return numberOfLevels_*c_pointerSize;
    }

    /** set of freeTableOffset - calculation done inside
    */
    void set_freeTableOffset()
    {
        size_t used_bytes_number=(lastIndex_+1)/8;
        freeTableOffset_=wholeBlockStart_+((used_bytes_number+(used_bytes_number%smallestAllocatableBlockSizeInBytes_))/(smallestAllocatableBlockSizeInBytes_*c_pointerSize));
    }

    /** set of freeTableOffset
    \param void** fOffset
    */
    void set_freeTableOffset(void** fOffset)
    {
        freeTableOffset_=fOffset;
    }

    /** set of splitTableOffset - calculation done inside
    */
    void set_splitTableOffset()
    {
        size_t used_bytes_number=(lastIndex_+1)/16;
        splitTableOffset_=freeTableOffset_+((used_bytes_number+(used_bytes_number%smallestAllocatableBlockSizeInBytes_))/(smallestAllocatableBlockSizeInBytes_*c_pointerSize));
    }

    /** set of splitTableOffset
    \param void** sOffset
    */
    void set_splitTableOffset(void** sOffset)
    {
        splitTableOffset_=sOffset;
    }

    /** set of lastIndex - calculation done inside
    */
    void set_lastIndex()
    {
        lastIndex_=powerOfTwo(numberOfLevels_)-2;
    }

    /** set of smallestAllocatableBlockSizeExponent - calculation done inside
    * the value is chosen to be such in order to use maximum two leafs
    * for the bookkeeping - without taking in mind that there could be non-bitfield or non-pointer offset
    */
    void set_smallestAllocatableBlockSizeExponent()
    {
        if(wholeBlockSizeExponent_%2==0)
            smallestAllocatableBlockSizeExponent_ = (wholeBlockSizeExponent_-2)/2; //the size is chosen
//to be such in order to be used no more than 2 blocks for the free and split tables offsets
        else
            smallestAllocatableBlockSizeExponent_ = (wholeBlockSizeExponent_-1)/2;
    }

    /** set of smallestAllocatableBlockSizeInBytes - calculation done inside
    */
    void set_smallestAllocatableBlockSizeInBytes()
    {
        smallestAllocatableBlockSizeInBytes_=powerOfTwo(smallestAllocatableBlockSizeExponent_);
    }

    /** set of whole block size according to bookkeeping - calculation done inside
    */
    void set_wholeBlockSizeAccordingToBookkeeping()
    {
        wholeBlockSizeAccordingToBookkeeping_=powerOfTwo(wholeBlockSizeExponent_);
    }

    /** set of number of levels - calculation done inside
    */
    void set_numberOfLevels()
    {
        numberOfLevels_=wholeBlockSizeExponent_-smallestAllocatableBlockSizeExponent_+1;
    }

    /** set of isAllocated bit by block index
    *\param size_t blockIndex
    *\param bool allocated
    */
    void set_allocated(size_t blockIndex, bool allocated)
    {
        if(blockIndex<=lastIndex_ && isAllocated(blockIndex)!=allocated)
        {
            unsigned short blockIndexInByte=blockIndex%8;
            size_t byteNumber=blockIndex/8; //byte number in the free table bitfield
            char* byte = freeTableBitfield_+byteNumber;
            if(allocated)
                *byte =(1U << blockIndexInByte) | *byte; //the bit of the block is set to 1,
            // others in byte to 0 and it is compared with the content of *byte
            else *byte= ~(1U<<blockIndexInByte)& *byte; //the bit of the block is set to 0,
            // others in byte to 1 and it is compared with the content of *byte
        }
    }

    /** set of isSplit bit by block index
    *\param size_t blockIndex
    *\param bool split
    */
    void set_split(size_t blockIndex, bool split)
    {
        if(blockIndex<=lastIndex_/2&&(isSplit(blockIndex)!=split))
        {
            unsigned short blockIndexInByte=blockIndex%8;
            size_t byteNumber=blockIndex/8; //byte number in the free table bitfield
            char* byte = splitTableBitfield_+byteNumber;
            if(split)
                *byte =(1U << blockIndexInByte) | *byte; //the bit of the block is set to 1,
            // others in byte to 0 and it is compared with the content of *byte
            else
                *byte= ~(1U<<blockIndexInByte)& *byte;//the bit of the block is set to 0,
            // others in byte to 1 and it is compared with the content of *byte
        }
    }

    /** set the pointer of the first free block of level blockLevel to blockAddress
    *\param unsigned short blockLevel
    *\param void** blockAddress
    */
    void set_thePointerToTheFirstFreeBlockOfLevel(unsigned short blockLevel,void** blockAddress)
    {
        *( splitTableOffset_+blockLevel)=blockAddress;
    }

//
// help functions
//


    /** Removes the block with index blockIndex from the free list of its level
    * @param size_t blockIndex
    */
    void removeBlockFromTheFreeListIndex(size_t blockIndex);

    /** Checks how many levels upward is the free larger block
    * @param unsigned short newAllocationSizeExponent
    *\return unsigned short the number of levels upward
    */
    unsigned short howManyLevelsUpwardIsTheFreeLargerBlock(unsigned short newAllocationSizeexponent);

    /** Adds the block with index blockIndex to the free list of its level
    * @param size_t blockIndex
    */
    void addBlockInTheFreeList(size_t blockIndex);

    /** Removes the block that is first in the free list of level blockLevel and returns its index
    * @param unsigned short blockLevel
    *\return size_t the index of the first free block of that level
    */
    size_t removeTheFirstBlockInTheFreeListAndReturnItsIndex(unsigned short blockLevel);


//
//initializations
//


    /** First initialize - decides whether buddy with slab allocator will be used or only slab allocator and
    */
    void initialize();

    /** Second initialize - Initialization of the buddy allocator
    * initializes all of the member variables and prepares the bookkeeping
    */
    void initializeBuddy();

    /** Allocates memory and initialize the bitfields for the bookkeeping
    * @param size_t usedLeafsNumber - how much leafs (block with smallest allocatable size) are necessary
    */
    void allocateMemoryForTheBookkeeping(size_t usedLeafsNumber);

    /** Called by initializeBuddy
    * initializes the pointers to first free blocks of the levels
    */
    void initialiseThePointers();

    /** Called by initializeBuddy
    * initializes the bitfields (isAllocated and isSplit)
    *\return the number of bytes used - sent to allocateMemoryForTheBookkeeping
    */
    size_t initialiseTheBitfieldsAndReturnTheUsedBytes();
//
//core functions
//

public:

    /** Allocates a block with at least newAllocationSize
    * finds the exponent of the closest larger power of two than the requested block size and returns a pointer with block with this size
    *\param size_t newAllocationSize - the requested block size
    *\return void* pointer to the allocated memory
    */
    void* allocate(size_t newAllocationSize);

    /** Deallocates the block on address pointer
    * uses indexOfAllocatedByPointer(pointer) to find which is the allocated block containing this address
    *\param void* pointer - pointer to the memory that has to be deallocated
    *\return bool if memory was deallocated
    */
    bool deallocate(void* pointer);

private:

    /** Called by allocate
    *allocates block with powerOfTwo(newAllocation_Exponent) size
    *\param size_t newAllocationSizeExponent - the exponent of the closest larger power of two than the requested block size
    *\return void* pointer to the allocated memory
    */
    void* allocateExponentSize(unsigned short newAllocationSizeExponent);

    /** Splits the first block in the free list of level level
    *\param unsigned short level
    */
    void split(unsigned short level);

    /** Called by allocateExponentSize
    * splits blocks upward until there is a free block with the certain size (newAllocationSizeexponent)
    * and then calls allocateExponentSize
    *\param size_t newAllocationSizeExponent - the requested block size
    *\return void* pointer to the allocated memory
    */
    void* splitUntilThereIsAFreeBlockWithThisSize(unsigned short newAllocationSizeExponent);



    /** Called by deallocate
    * checks if the buddies can be coalesced or only to add the newly deallocated block to the free list of its level
    * and then calls the needed function
    *\param size_t blockIndex - the index of the deallocated block
    */
    void coalesceOrAddToTheFreeList(size_t blockIndex);

    /** Called by coalesceOrAddToTheFreeList
    * Coalesce the buddies, removes them from the free list of their level and add their parent to this list
    *\param size_t leftBuddyIndex - the index of the left buddy
    */
    void coalesce(size_t leftBuddyIndex);


//
//member variables
//


    void** freeTableOffset_; //!< Member variable "freeTableOffset"
    void** splitTableOffset_; //!< Member variable "splitTableOffset"
    void** bookkeepingOffset_; //!< Member variable "bookkeepingOffset"
    void** wholeBlockStart_; //!< Member variable "wholeBlockStart"
    void** wholeBlockStartAccordingToBookkeeping_; //!< Member variable "wholeBlockStartAccordingToBookkeeping"
    char* freeTableBitfield_; //!< Member variable "freeTableBitfield"
    char* splitTableBitfield_; //!< Member variable "splitTableBitfield"
    size_t wholeBlockSize_; //!< Member variable "wholeBlockSize"
    size_t wholeBlockSizeAccordingToBookkeeping_; //!< Member variable "wholeBlockSizeAccordingToBookkeeping"
    unsigned short wholeBlockSizeExponent_; //!< Member variable "wholeBlockSizeExponent_"
    unsigned short smallestAllocatableBlockSizeExponent_; //!< Member variable "smallestAllocatableBlockSizeexponent"
    size_t smallestAllocatableBlockSizeInBytes_;//!< Member variable "smallestAllocatableBlockSizeInBytes"
    unsigned short numberOfLevels_; //!< Member variable "numberOfLevels"
    bool pureSlab_;  //!< Member variable "pureSlab"
    size_t lastIndex_; //!< Member variable "lastIndex"
};

#endif // ALLOCATORH
