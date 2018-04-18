#include <thread>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#include "libcds.h"
#include <cds/container/vyukov_mpmc_cycle_queue.h>

#include "Engine.h"
#include "PerfLog.h"

#include "LocklessList.h"
#include "MemoryPool.h"

#include "NotePlayHandle.h"

template<typename T>
using LocklessQueue = cds::container::VyukovMPMCCycleQueue<T>;

template<typename Alloc>
void benchmark_allocator(QString name, Alloc&& alloc, size_t n, size_t I)
{
	using T = typename Alloc::value_type;
	constexpr size_t S = sizeof(T);

	std::vector<T*> ptrs{n};
	PerfLogTimer timer(QString("Allocate: %1 x %2 x %3 bytes, %4")
					   .arg(I).arg(n).arg(S).arg(name));

	for (size_t i=0; i < I; i++)
	{
		for (size_t j=0; j < n; j++) {
			ptrs[j] = alloc.allocate(1);
		}
		for (size_t j=0; j < n; j++) {
			alloc.deallocate(ptrs[j], 1);
		}
	}
}


template<typename Alloc>
void benchmark_allocator_threaded(QString name, Alloc&& alloc, size_t n, size_t t)
{
	using T = typename Alloc::value_type;
	constexpr size_t S = sizeof(T);

	LocklessQueue<T*> ptrs{n};

	PerfLogTimer timer(QString("Allocate multi-threaded: %1 x %2 bytes using %3 threads, %4")
					   .arg(n).arg(S).arg(t).arg(name));

	std::vector<std::thread> threads; threads.reserve(t*2);

	std::atomic_uint_fast64_t allocated{0};
	std::atomic_uint_fast64_t deallocated{0};

	for (size_t i=0; i < t; i++) {
		threads.emplace_back([&]() {
			while(allocated++ < n) {
				auto ptr = alloc.allocate(1);
				ptrs.push(ptr);
			}
		});
	}
	for (size_t i=0; i < t; i++) {
		threads.emplace_back([&]() {
			while(deallocated++ < n) {
				T* ptr;
				while (! ptrs.pop(ptr));
				alloc.deallocate(ptr, 1);
			}
		});
	}

	for (std::thread& thread : threads) {
		thread.join();
	}
}

int main(int argc, char* argv[])
{
	new QCoreApplication(argc, argv);

	using Stack = LocklessList<size_t>;
	{
		size_t n = 100 * 1000 * 1000;
		Stack stack{n};
		PerfLogTimer timer("LocklessList: Insert 100m entries, single-threaded, pre-allocated");
		for (size_t i=0; i < n; i++) {
			stack.push(i);
		}
	}
	{
		size_t n = 50 * 1000 * 1000;
		size_t t = 5;
		Stack stack{n};
		std::vector<std::thread> threads; threads.reserve(t);
		PerfLogTimer timer("LocklessList: Push 50m entries, multi-threaded, pre-allocated");

		for (int i=0; i < 5; i++) {
			threads.emplace_back([&]() {
				for (size_t j=0; j < n / t; j++) {
					stack.push(j);
				}
			});
		}

		for (int i=0; i < 5; i++) {
			threads.at(i).join();
		}
	}

	{
		size_t n = 10 * 1000 * 1000;
		constexpr size_t S = 256;
		using T = std::array<char, S>;
		benchmark_allocator("std::allocator", std::allocator<T>{}, n, 1);
		benchmark_allocator("MmAllocator", MmAllocator<T>{}, n, 1);
		benchmark_allocator("MemoryPool", MemoryPool<T>{n}, n, 1);
	}
	{
		size_t n = 10 * 1000 * 1000;
		constexpr size_t S = 256;
		using T = std::array<char, S>;
		benchmark_allocator_threaded("std::allocator", std::allocator<T>{}, n, 4);
		benchmark_allocator_threaded("MmAllocator", MmAllocator<T>{}, n, 4);
		benchmark_allocator_threaded("MemoryPool", MemoryPool<T>{n}, n, 4);
	}
}
