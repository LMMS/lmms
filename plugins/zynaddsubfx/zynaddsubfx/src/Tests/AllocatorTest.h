/*
  ZynAddSubFX - a software synthesizer

  AllocatorTest.h - CxxTest for RT Memory Allocator
  Copyright (C) 2014-2014 Mark McCurry

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/


#include <cxxtest/TestSuite.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <cstring>
#include <string>
#include <vector>
#include "../Misc/Allocator.h"
//using namespace std;
using std::vector;
class AllocatorTest:public CxxTest::TestSuite
{
    public:
        Allocator     *memory_;
        vector<void*> data;

        void setUp() {
            memory_ = new Allocator();
        }

        void tearDown() {
            delete memory_;
        }

        void testBasic() {
            Allocator &memory = *memory_;
            char *d = (char*)memory.alloc_mem(128);
            TS_ASSERT(d);
            d[0]   = 0;
            d[127] = 0;
            memory.dealloc_mem(d);
        }

        void testTooBig() {
            Allocator &memory = *memory_;
            //Try to allocate a gig
            char *d = (char*)memory.alloc_mem(1024*1024*1024);
            TS_ASSERT(d==nullptr);
        }

        void testEnlarge()
        {
            Allocator &memory = *memory_;
            //Additional Buffers
            size_t N = 50*1024*1024;
            char *bufA = (char*)malloc(N);
            char *bufB = (char*)malloc(N);
            memset(bufA,0xff, N);
            memset(bufB,0xff, N);


            //By default 25MBi is too large
            //Therefore this allocation should fail
            bool low = memory.lowMemory(5,5*1024*1024);
            TS_ASSERT(low);
            TS_ASSERT(memory.memPools() == 1);


            //Try to add a buffer
            //This provides enough for the low memory check to pass
            memory.addMemory(bufA, N);
            TS_ASSERT(memory.memPools() == 2);
            TS_ASSERT(memory.memFree(bufA));
            bool low2 = memory.lowMemory(5,5*1024*1024);
            TS_ASSERT(!low2);
            TS_ASSERT(memory.memFree(bufA));

            //We should be able to see that a chunk enters and exits the free
            //state
            char *mem2 = (char*)memory.alloc_mem(10*1024*1024);
            TS_ASSERT(mem2);
            TS_ASSERT(!memory.memFree(bufA));
            memory.dealloc_mem(mem2);
            TS_ASSERT(memory.memFree(bufA));
            mem2 = (char*)memory.alloc_mem(10*1024*1024);
            char *mem3 = (char*)memory.alloc_mem(10*1024*1024);
            TS_ASSERT(mem3);
            memory.dealloc_mem(mem2);
            TS_ASSERT(!memory.memFree(bufA));
            TS_ASSERT(memory.freePools() == 0);
            memory.dealloc_mem(mem3);
            TS_ASSERT(memory.memFree(bufA));
            TS_ASSERT(memory.freePools() == 1);

            //Observe that adding another pool superficially works
            memory.addMemory(bufB, N);
            TS_ASSERT(memory.freePools() == 2);

            //delete [] bufA;
            //delete [] bufB;
        }

};
