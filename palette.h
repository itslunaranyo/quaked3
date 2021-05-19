#ifndef __H_PALETTE
#define __H_PALETTE

#include "qeBuffer.h"

class Palette
{
public:
	Palette();
	~Palette();

	typedef struct {
		byte r, g, b, a;
		unsigned AsInt() { return ((unsigned int)255 << 24) + (b << 16) + (g << 8) + r; }
	} color_t;

	void LoadFromFile(const char* palfile);

	byte Red(byte i) { return pal[i].r; }
	byte Green(byte i) { return pal[i].g; }
	byte Blue(byte i) { return pal[i].b; }
	unsigned ColorAsInt(byte i) { return pal[i].AsInt(); }
	void ColorAsVec3(byte i, vec3_t c);

private:
	bool LoadFromFileImpl(const char* file);
	void GenerateErrorPalette();
	void GenerateGammaTable(byte* table);
	color_t pal[256];
};

#endif	// __H_PALETTE