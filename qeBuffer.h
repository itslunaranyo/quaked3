//==============================
//	qeBuffer.h
//==============================

#ifndef __H_QE_BUFFER__
#define __H_QE_BUFFER__

#include <cassert>
#include <cstring>	// for std::memset

typedef unsigned char byte;

class qeBuffer
{
public:
	// todo: all the other constructors
	qeBuffer(size_t size = 0) : len(size), buf(nullptr) { create(); }
	qeBuffer(const qeBuffer& other) : len(other.len), buf(nullptr)
	{
		create();
		std::memcpy(buf, other.buf, len);
	}
	~qeBuffer() { destroy(); }

	size_t size() const { return len; }
	void* operator*() const { return reinterpret_cast<void*>(buf); }
//	operator char*() const { return reinterpret_cast<char*>(buf); }	// this is not a string class, use c_str() manually if you want to pretend it is
	byte& operator[](unsigned i) { assert(i < len); return buf[i]; }
	byte& operator[](unsigned i) const { assert(i < len); return buf[i]; }

	bool operator==(const char* other) const { return ( strncmp((char*)buf, other, len) == 0 ); }
	bool operator!=(const char* other) const { return (strncmp((char*)buf, other, len) != 0); }
	qeBuffer& operator=(const char* other) {
		resize(strlen(other) + 1);
		std::memcpy(buf, other, len);
		return *this;
	}

	bool operator==(const qeBuffer& other) const { return ( len == other.len && ((char*)buf, (char*)*other, len) == 0 ); }
	bool operator!=(const qeBuffer& other) const { return !( *this == other ); }
	qeBuffer& operator=(const qeBuffer& other) {
		resize(other.len);
		std::memcpy(buf, other.buf, len);
		return *this;
	}

	char* const c_str() { return reinterpret_cast<char*>(buf); }	// TODO: use this in all the places it should be
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