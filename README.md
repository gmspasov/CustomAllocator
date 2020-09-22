# CustomAllocator
Buddy Allocator with preparation for addition of Slab Allocator. Seminar of System Programming course project.

 /**Created by
    * \author Georgi Spasov
    * e-mail : gmspasov@uni-sofia.bg | gecta13@gmail.com
    * on 04.09.2020 20:35 as System Programming Seminar project.
    *
    *Last modification 22.09.2020 by Georgi Spasov.
    *
    * Custom allocator using Buddy Algorithm for big allocations,
    * for small allocations it is prepared for the implementation of
    * slab allocator algorithm BUT IT HAS NOT BEEN IMPLEMENTED yet.
    * Buddy memory allocation : https://en.wikipedia.org/wiki/BuddyMemoryAllocation
    * Slab memory allocation : https://en.wikipedia.org/wiki/SlabAllocation
    * Briefly this implementation of buddy allocator is following the classical idea.
    * It operates with a block allocated at first with malloc()
    * and when request of certain size of memory comes the block is divided (split) in
    * power of two sized blocks with the suitable size (if there is enough free memory) and
    * then pointer to that certain free block is returned (for 595 bytes it would be block with
    * 1024 bytes).There is limit of how small block can be to be allocated and there is bookkeeping
    * in which is written which are the free blocks or those who have been split (a block of 2048
    * can be divided in two of 1024 in order to avoid losses. Deallocation also is provided. The
    * information of the bookkeeping is written inside the block (at the beginning), I decided to
    * choose the limit of how small allocatable block to have with the mind of that the information
    * of the bookkeeping to fit in no more than 2 block with minimal size.
    * If the size of the whole block is not power of two - the bookkeeping works with illusion, for
    * it the size always a power of two (the difference between the two sizes is said to be constantly
    * allocated). When a block is split the left "buddy" is allocated and the right is put in the
    * corresponding list of free blocks of that level (with that size). Blocks are identified by
    * indexes.In every block is written its next and PREVIOUS free block in free list. Previous is
    * needed when block is removed.
    *
    * For more information you can see:
    * the Internet, C++ conferences and the video provided by my university professors :
    *https://www.youtube.com/watch?v=xXvyn6Oz7gI (in Bulgarian).I've used also information from
    *en.cppreference.com, www.geeksforgeeks.org, www.quora.com and stackoverflow.com.
    */
