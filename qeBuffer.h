//==============================
//	qeBuffer.cpp
//==============================

#ifndef __H_QE_BUFFER__
#define __H_QE_BUFFER__

#include <cassert>
#include <cstring>	// for std::memset

typedef unsigned char byte;

class qeBuffer
{
public:
	qeBuffer(size_t size = 0) : len(size), buf(nullptr) { create(); }
	~qeBuffer() { destroy(); }

	size_t size() { return len; }
	void* operator*() const { return reinterpret_cast<void*>(buf); }
	byte& operator[](unsigned i) { assert(i < len); return buf[i]; }
	byte& operator[](unsigned i) const { assert(i < len); return buf[i]; }

	void resize(size_t size) { 
		destroy(); 
		len = size;
		create();
	}
	void zero() { 
		//for (size_t i = 0; i < len; i++) buf[i] = 0; 
		std::memset(buf, 0, len);
	}
	
private:
	void destroy() {
		if (!len) return;
		assert(buf);
		delete[] buf;
		buf = nullptr;
		len = 0; 
	}
	void create() {
		if (!len) return;
		assert(!buf); 
		buf = new byte[len];
	}

	size_t len;
	byte* buf;
};

#endif	// __H_QE_BUFFER__