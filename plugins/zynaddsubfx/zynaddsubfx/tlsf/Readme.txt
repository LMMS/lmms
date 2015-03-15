Two Level Segregated Fit memory allocator implementation.
Written by Matthew Conte (matt@baisoku.org).
Public Domain, no restrictions.

	http://tlsf.baisoku.org

Features
--------
* O(1) cost for malloc, free, realloc, memalign
* Extremely low overhead per allocation (4 bytes)
* Low overhead per TLSF management of pools (~3kB)
* Low fragmentation
* Compiles to only a few kB of code and data
* Support for adding and removing memory pool regions on the fly

Caveats
-------
* Currently, assumes architecture can make 4-byte aligned accesses
* Not designed to be thread safe; the user must provide this

Notes
-----
This code was based on the TLSF 1.4 spec and documentation found at:

	http://rtportal.upv.es/rtmalloc/allocators/tlsf/index.shtml

It also leverages the TLSF 2.0 improvement to shrink the per-block overhead
from 8 to 4 bytes.

Known Issues
------------
* Due to the internal block structure size and the implementation
details of tlsf_memalign, there is worst-case behavior when requesting
small (<16 byte) blocks aligned to 8-byte boundaries. Overuse of memalign
will generally increase fragmentation, but this particular case will leave
lots of unusable "holes" in the pool. The solution would be to internally
align all blocks to 8 bytes, but this will require significantl changes
to the implementation. Contact me if you are interested.

History
-------
2014/02/08 - v3.0
   * This version is based on improvements from 3DInteractive GmbH
   * Interface changed to allow more than one memory pool
   * Separated pool handling from control structure (adding, removing, debugging)
   * Control structure and pools can still be constructed in the same memory block
   * Memory blocks for control structure and pools are checked for alignment
   * Added functions to retrieve control structure size, alignment size, min and
     max block size, overhead of pool structure, and overhead of a single allocation
   * Minimal Pool size is tlsf_block_size_min() + tlsf_pool_overhead()
   * Pool must be empty when it is removed, in order to allow O(1) removal

2011/10/20 - v2.0
   * 64-bit support
   * More compiler intrinsics for ffs/fls
   * ffs/fls verification during TLSF creation in debug builds

2008/04/04 - v1.9
   * Add tlsf_heap_check, a heap integrity check
   * Support a predefined tlsf_assert macro
   * Fix realloc case where block should shrink; if adjacent block is
     in use, execution would go down the slow path

2007/02/08 - v1.8
   * Fix for unnecessary reallocation in tlsf_realloc

2007/02/03 - v1.7
   * tlsf_heap_walk takes a callback
   * tlsf_realloc now returns NULL on failure
   * tlsf_memalign optimization for 4-byte alignment
   * Usage of size_t where appropriate

2006/11/21 - v1.6
   * ffs/fls broken out into tlsfbits.h
   * tlsf_overhead queries per-pool overhead

2006/11/07 - v1.5
   * Smart realloc implementation
   * Smart memalign implementation

2006/10/11 - v1.4
   * Add some ffs/fls implementations
   * Minor code footprint reduction

2006/09/14 - v1.3
   * Profiling indicates heavy use of blocks of
     size 1-128, so implement small block handling
   * Reduce pool overhead by about 1kb
   * Reduce minimum block size from 32 to 12 bytes
   * Realloc bug fix

2006/09/09 - v1.2
   * Add tlsf_block_size
   * Static assertion mechanism for invariants
   * Minor bugfixes 

2006/09/01 - v1.1
   * Add tlsf_realloc
   * Add tlsf_walk_heap

2006/08/25 - v1.0
   * First release
