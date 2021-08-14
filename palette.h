#ifndef __H_PALETTE
#define __H_PALETTE

class Palette {
public:
	Palette() {}
	~Palette() {}

	struct color_t {
		byte r, g, b, a;
		unsigned AsInt() { return ((unsigned int)255 << 24) + (b << 16) + (g << 8) + r; }
	};

	inline byte Red(byte i) { return pal[i].r; }
	inline byte Green(byte i) { return pal[i].g; }
	inline byte Blue(byte i) { return pal[i].b; }
	inline unsigned ColorAsInt(byte i) { return pal[i].AsInt(); }

	vec3 ColorAsVec3(const byte i);
private:
	friend class PaletteReader;
	color_t pal[256] = { 0 };
};

extern Palette g_palette;

#endif	// __H_PALETTE