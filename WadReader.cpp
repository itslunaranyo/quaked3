#include "pre.h"
#include "BinaryFileReader.h"
#include "WadReader.h"
#include "StringFormatter.h"
#include "Texture.h"
#include "Textures.h"
#include "TextureGroup.h"

#include "Palette.h"
#include "qfiles.h"

//  .MIP file format
#define	TYP_MIPTEX	68
#define	MIPLEVELS	4

struct miptex_t
{
	char		name[MAX_TEXNAME];
	unsigned	width, height;
	unsigned	offsets[MIPLEVELS];	// four mip maps stored
};

struct wadinfo_t
{
	char	identification[4];	// should be WAD2 or 2DAW
	int		numlumps;
	int		infotableofs;
};

struct lumpinfo_t
{
	int		filepos;
	int		disksize;
	int		size;				// uncompressed
	char	type;
	char	compression;
	char	pad1, pad2;
	char	name[16];			// must be null terminated
};

//=============================================================================



/*
==================
WadReader::Read
==================
*/
TextureGroup* WadReader::Read(const std::string& filename)
{
	BinaryFileReader fr;
	wadinfo_t *wadinfo;
	lumpinfo_t *lumps;
	std::vector<char> buf;
	std::vector<unsigned int> texbuf;
	char* bufp;

	if (!fr.Open(filename))
	{
		Log::Warning("Couldn't load wad!\n");
		return nullptr;
	}

	buf.resize(fr.Size());
	bufp = buf.data();

	fr.ReadAll(buf.data());
	wadinfo = (wadinfo_t*)bufp;

	if (strncmp(wadinfo->identification, "WAD2", 4))
	{
		Log::Warning("Couldn't load wad! (not a valid wadfile)\n");
		return false;
	}

	lumps = (lumpinfo_t*)(&buf[wadinfo->infotableofs]);
	TextureGroup* wad = new TextureGroup();
	wad->name = filename;

	for (int i = 0; i < wadinfo->numlumps; ++i)
	{
		if (lumps[i].type != TYP_MIPTEX)
		{
			Log::Warning(_S("%s is not a miptex, ignoring") << lumps[i].name);
			continue;
		}

		miptex_t* qmip = (miptex_t*)(&buf[lumps[i].filepos]);
		vec3 color;

		if (qmip->width > 1024 || qmip->width < 1 || qmip->height > 1024 || qmip->height < 1)
		{
			Log::Warning(_S("%s has bad size, ignoring") << lumps[i].name);
			continue;
		}

		if (lumps[i].filepos + (qmip->width * qmip->height) > fr.Size())
		{
			Log::Warning(_S("%s is incomplete, stopping after %i textures") << lumps[i].name << i);
			break;
		}

		unsigned int j, count;
		byte* src;
		color = vec3(0);

		src = (byte*)qmip + qmip->offsets[0];
		count = qmip->width * qmip->height;
		texbuf.resize(count);

		// The bitmaps in wads are upside down, which is right side up relative to a 
		// GL bottom-left-origin, so we can just copy straight
		for (j = 0; j < count; ++j)
		{
			texbuf[j] = g_palette.ColorAsInt(src[j]);
			color += g_palette.ColorAsVec3(src[j]);
		}
		color = color / (float)count;

		// instantiate a Texture and give it *miptex to draw parms and bitmap data from
		wad->Add(new Texture(qmip->width, qmip->height, qmip->name, color, (char*)texbuf.data()));
	}

	return wad;
}
