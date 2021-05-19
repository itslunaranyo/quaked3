#include "qe3.h"

Palette::Palette() {}
Palette::~Palette() {}

void Palette::LoadFromFile(const char* palfile)
{
	char pfname[MAX_PATH];
	qeBuffer lmp(768);

	Sys_Printf("Loading palette from %s ...\n", palfile);
	if (LoadFromFileImpl(pfname))
		return;

	Sys_Printf("Could not load %s, trying texturepath ...\n", palfile);
	sprintf(pfname, "%s/%s", g_project.wadPath, palfile);
	if (LoadFromFileImpl(pfname))
		return;

	Sys_Printf("Warning: Couldn't load palette! Loading local quakepal.lmp as fallback ...\n");
	if (LoadFromFileImpl("quakepal.lmp"))
		return;

	Sys_Printf("Warning: Couldn't open QE3 data directory! Are you running it from the directory it's installed in?\n");
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
		if (lenf <= 0)
			Sys_Printf("Warning: Couldn't open!\n");
		else
			Sys_Printf("Warning: Problem loading palette! (bad length)\n");
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
	return true;
}

void Palette::GenerateErrorPalette()
{
	Sys_Printf("Warning: Generating hideous error-colored palette ...\n");
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
	int		inf;
	float	gamma;

	gamma = (float)g_cfgUI.Gamma;

	if (gamma == 1.0)
	{
		for (i = 0; i < 256; i++)
			gammatable[i] = i;
	}
	else
	{
		for (i = 0; i < 256; i++)
		{
			inf = 255 * pow((i + 0.5) / 255.5, gamma) + 0.5;
			inf = max(0, min(255, inf));
			gammatable[i] = inf;
		}
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

