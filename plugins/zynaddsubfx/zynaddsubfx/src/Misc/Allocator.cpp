#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <utility>
#include <cstdio>
#include "../../tlsf/tlsf.h"
#include "Allocator.h"

//Used for dummy allocations
Allocator DummyAlloc;

//recursive type class to avoid void *v = *(void**)v style casting
struct next_t
{
    next_t *next;
    size_t pool_size;
};

void *data(next_t *n)
{
    return n+sizeof(next_t);
}


struct AllocatorImpl
{
    void *tlsf = 0;

    //singly linked list of memory pools
    //XXX this may violate alignment on some platforms if malloc doesn't return
    //nice values
    next_t *pools = 0;
    unsigned long long totalAlloced = 0;
};

Allocator::Allocator(void)
{
    impl = new AllocatorImpl;
	size_t default_size = 5*1024*1024; //curlymorphic was 5Mib
    impl->pools = (next_t*)malloc(default_size);
    impl->pools->next = 0x0;
    impl->pools->pool_size = default_size;
    size_t off = tlsf_size() + tlsf_pool_overhead() + sizeof(next_t);
    //printf("Generated Memory Pool with '%p'\n", impl->pools);
    impl->tlsf = tlsf_create_with_pool(((char*)impl->pools)+off, default_size-2*off);
    //printf("Allocator(%p)\n", impl);
}

Allocator::~Allocator(void)
{
    next_t *n = impl->pools;
    while(n) {
        next_t *nn = n->next;
        free(n);
        n = nn;
    }
    delete impl;
}

void *Allocator::alloc_mem(size_t mem_size)
{
	impl->totalAlloced += mem_size;
	void *mem = tlsf_malloc(impl->tlsf, mem_size);
	//printf("Allocator.malloc(%p, %d) = %p\n", impl, mem_size, mem);
//	void *mem = malloc(mem_size);
	//printf("Allocator result = %p\n", mem);
    return mem;
}
void Allocator::dealloc_mem(void *memory)
{
	//printf("dealloc_mem(%d)\n", tlsf_block_size(memory));
	tlsf_free(impl->tlsf, memory);
//	free(memory);
}

bool Allocator::lowMemory(unsigned n, size_t chunk_size)
{
    //This should stay on the stack
    void *buf[n];
    for(unsigned i=0; i<n; ++i)
        buf[i] = tlsf_malloc(impl->tlsf, chunk_size);
    bool outOfMem = false;
    for(unsigned i=0; i<n; ++i)
        outOfMem |= (buf[i] == nullptr);
    for(unsigned i=0; i<n; ++i)
        if(buf[i])
            tlsf_free(impl->tlsf, buf[i]);

    return outOfMem;
}


void Allocator::addMemory(void *v, size_t mem_size)
{
    next_t *n = impl->pools;
    while(n->next) n = n->next;
    n->next = (next_t*)v;
    n->next->next = 0x0;
    n->next->pool_size = mem_size;
    //printf("Inserting '%p'\n", v);
    off_t off = sizeof(next_t) + tlsf_pool_overhead();
    void *result =
        tlsf_add_pool(impl->tlsf, ((char*)n->next)+off,
                //0x0eadbeef);
            mem_size-off-sizeof(size_t));
    if(!result)
        printf("FAILED TO INSERT MEMORY POOL\n");
};//{(void)mem_size;};

//From tlsf internals
typedef struct block_header_t
{
	/* Points to the previous physical block. */
	struct block_header_t* prev_phys_block;

	/* The size of this block, excluding the block header. */
	size_t size;

	/* Next and previous free blocks. */
	struct block_header_t* next_free;
	struct block_header_t* prev_free;
} block_header_t;
static const size_t block_header_free_bit = 1 << 0;

bool Allocator::memFree(void *pool)
{
    size_t bh_shift = sizeof(next_t)+sizeof(size_t);
    //Assume that memory is free to start with
    bool isFree = true;
    //Get the block header from the pool
    block_header_t &bh  = *(block_header_t*)((char*)pool+bh_shift);
    //The first block must be free
    if((bh.size&block_header_free_bit) == 0)
        isFree = false;
    block_header_t &bhn = *(block_header_t*)
        (((char*)&bh)+((bh.size&~0x3)+bh_shift-2*sizeof(size_t)));
    //The next block must be 'non-free' and zero length
    if((bhn.size&block_header_free_bit) != 0)
        isFree = false;
    if((bhn.size&~0x3) != 0)
        isFree = false;

    return isFree;
}

int Allocator::memPools()
{
    int i = 1;
    next_t *n = impl->pools;
    while(n->next) {
        i++;
        n = n->next;
    }
    return i;
}

int Allocator::freePools()
{
    int i = 0;
    next_t *n = impl->pools->next;
    while(n) {
        if(memFree(n))
            i++;
        n = n->next;
    }
    return i;
}


unsigned long long Allocator::totalAlloced()
{
    return impl->totalAlloced;
}

/*
 * Notes on tlsf internals
 * - TLSF consists of blocks linked by block headers and these form a doubly
 *   linked list of free segments
 * - Original memory is [control_t pool]
 *              base    sentinal
 *   Pools are [block_t block_t blocks ...]
 *   Blocks are [memory block_t](??)
 * - These are stored in the control_t structure in an order dependent on the
 *   size that they are
 *   it's a bit unclear how collisions are handled here, but the basic premise
 *   makes sense
 * - Additional structure is added before the start of each pool to define the
 *   pool size and the next pool in the list as this information is not
 *   accessible in O(good) time
 */
