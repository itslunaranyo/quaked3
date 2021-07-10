//==============================
//	palette.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "palette.h"

Palette::Palette() {}
Palette::~Palette() {}

void Palette::LoadFromFile(const char* palfile)
{
	char pfname[MAX_PATH];
	qeBuffer lmp(768);

	Log::Print(_S("Loading palette from %s ...\n")<< palfile);
	if (LoadFromFileImpl(pfname))
		return;

	Log::Print(_S("Could not load %s, trying texturepath ...\n")<< palfile);
	sprintf(pfname, "%s/%s", g_project.wadPath, palfile);
	if (LoadFromFileImpl(pfname))
		return;

	Log::Warning("Couldn't load palette! Loading local quakepal.lmp as fallback ...");
	if (LoadFromFileImpl("./quakepal.lmp"))
		return;

	Log::Warning("Couldn't open QE3 data directory! Are you running it from the directory it's installed in?");
	GenerateErrorPalette();
}

bool Palette::LoadFromFileImpl(const char* file)
{
	int lenf;
	qeBuffer lmp(768);
	byte	gammatable[256];

	lenf = IO_LoadFile(file, lmp);
	if (lenf != 768)
	{
		if (lenf > 0)
			Log::Warning("Problem loading palette! (bad length)");
		return false;
	}

	int p = 0;
	byte r, g, b;
	byte* lmppal = (byte*)*lmp;
	GenerateGammaTable(gammatable);
	for (int i = 0; i < 256; i++)
	{
		r = lmppal[p++];
		g = lmppal[p++];
		b = lmppal[p++];

		pal[i] = { gammatable[r], gammatable[g], gammatable[b], 255};
	}
	Log::Print("Palette loaded.\n");
	return true;
}

void Palette::GenerateErrorPalette()
{
	Log::Warning("Generating hideous error-colored palette ...");
	int i;
	for (i = 0; i < 256; i++)
	{
		byte x = (byte)(i % 255);
		pal[i] = { x, 0, x, 255 };
	}
}

void Palette::GenerateGammaTable(byte *gammatable)
{
	int		i;
	float	v;
	int		inf;

	for (i = 0; i < 256; i++)
	{
		v = min(255.0f, (float)i * (float)g_cfgUI.Brightness);
		v = 255 * pow((v + 0.5f) / 255.5, (float)g_cfgUI.Gamma) + 0.5f;
		inf = ceil(max(0.0f, min(255.0f, v)));
		gammatable[i] = inf;
	}

}

vec3 Palette::ColorAsVec3(const byte i)
{
	vec3 c;
	c[0] = pal[i].r / 255.0f;
	c[1] = pal[i].g / 255.0f;
	c[2] = pal[i].b / 255.0f;
	return c;
}

