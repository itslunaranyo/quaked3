#ifndef __H_SLAB_ALLOCATOR
#define __H_SLAB_ALLOCATOR

#include <stack>

// this is that CRTP hunk chunk slab block lump heap allocator that's good for cache coherence

template <typename T, size_t PoolSize = 64, size_t SlabSize = 256>
class SlabAllocator
{
private:
	class Slab
	{
	private:
		byte slab[SlabSize * sizeof(T)];
		byte firstFree;
		byte numFree;

	public:
		Slab() : firstFree(0), numFree(SlabSize-1)
		{
			assert(SlabSize - 1 < 256);	// don't overflow the byte index
			// initialize the byte at the start of each block to the index of the next empty one
			for (size_t i = 0; i < SlabSize - 1; i++)
			{
				slab[i * sizeof(T)] = static_cast<byte>(i + 1);
			}
		}

		T* allocate()
		{
			assert(numFree);

			byte* out = slab + (firstFree * sizeof(T));
			firstFree = *out;
			numFree--;

			return reinterpret_cast<T*>(out);
		}

		void deallocate(void* d)
		{
			assert(contains(d));
			assert(aligned(d));
			assert(numFree < SlabSize - 1);

			byte* db = reinterpret_cast<byte*>(d);
			byte index = static_cast<byte>((db - slab) / sizeof(T));
			*db = firstFree;
			firstFree = index;
			numFree++;
		}

		bool contains(const void* d)
		{
			const byte* db = reinterpret_cast<const byte*>(d);
			if (db < slab) return false;
			if (db >= slab + (SlabSize - 1) * sizeof(T)) return false;
			return true;
		}

		bool aligned(const void* d)
		{
			const byte* db = reinterpret_cast<const byte*>(d);
			if ((db - slab) % sizeof(T)) return false;
			return true;
		}

		bool empty() { return (numFree == SlabSize - 1); }
		bool full()  { return (numFree == 0); }
	};

	typedef std::vector<Slab*> SlabList;
	typedef std::stack<T*> Pool;

	static Pool& pool()
	{
		static Pool p;
		return p;
	}
	static SlabList& fullSlabs()
	{
		static SlabList full;
		return full;
	}
	static SlabList& mixedSlabs()
	{
		static SlabList mixed;
		return mixed;
	}
	static SlabList& emptySlabs()
	{
		static SlabList empty;
		return empty;
	}

public:
	void* operator new(size_t size)
	{
		assert(size == sizeof(T));
		
		T* out;

		if (!pool().empty())
		{
			out = pool().top();
			pool().pop();
			return out;
		}
		
		Slab* slab;
		if (!mixedSlabs().empty())
		{
			slab = mixedSlabs().back();
			mixedSlabs().pop_back();
		}
		else if (!emptySlabs().empty())
		{
			slab = emptySlabs().back();
			emptySlabs().pop_back();
		}
		else
		{
			slab = new Slab();
		}

		out = slab->allocate();

		if (slab->full())
			fullSlabs().push_back(slab);
		else
			mixedSlabs().push_back(slab);

		return out;
	}

	void operator delete(void* d)
	{
		T* p = reinterpret_cast<T*>(d);
		if (PoolSize && pool().size() < PoolSize)
		{
			pool().push(p);
			return;
		}

		// find the slab it's in & remove it
		// FIXME: for large deletes this is slow as fuckballs
		Slab *home = NULL;
		SlabList::iterator h;
		for (SlabList::reverse_iterator mixedIt = mixedSlabs().rbegin(); mixedIt < mixedSlabs().rend(); mixedIt++)
		{
			if ((*mixedIt)->contains(p))
			{
				h = (mixedIt + 1).base();
				home = *h;
			//	std::iter_swap(h, mixedSlabs().end() - 1);
			//	mixedSlabs().erase(mixedSlabs().end() - 1);
				mixedSlabs().erase(h);
				break;
			}
		}
		if (!home)
		{
			for (SlabList::reverse_iterator fullIt = fullSlabs().rbegin(); fullIt < fullSlabs().rend(); fullIt++)
			{
				if ((*fullIt)->contains(p))
				{
					h = (fullIt + 1).base();
					home = *h;
				//	std::iter_swap(h, fullSlabs().end() - 1);
				//	fullSlabs().erase(fullSlabs().end() - 1);
					fullSlabs().erase(h);
					break;
				}
			}
		}
		assert(home);
		// deallocate
		home->deallocate(p);

		// push to appropriate list
		if (home->empty())
		{
			emptySlabs().push_back(home);
		}
		else
		{
			mixedSlabs().push_back(home);
		}
	}


};

#endif