#include "pre.h"
#include "Texture.h"
#include "TextureGroup.h"
#include "Textures.h"
#include <random>
/*
==================
TextureGroup::TextureGroup
==================
*/
TextureGroup::TextureGroup() : first(nullptr), numTextures(0), flushed(false), name("?"), id(rand())
{	
}

/*
==================
TextureGroup::~TextureGroup
==================
*/
TextureGroup::~TextureGroup()
{
	Flush();
}

/*
==================
TextureGroup::Flush
==================
*/
void TextureGroup::Flush()
{
	Texture* tex, * nexttex;
	numTextures = 0;
	for (tex = first; tex; tex = nexttex)
	{
		nexttex = tex->next;
		delete tex;
	}
	first = nullptr;
}

/*
==================
TextureGroup::ByColor

a quick way to find entity solid color textures
==================
*/
Texture* TextureGroup::ByColor(const vec3 oc)
{
	for (Texture* tex = first; tex; tex = tex->next)
	{
		if (VectorCompare(tex->color, oc))
			return tex;
	}
	return nullptr;
}

/*
==================
TextureGroup::ForName
==================
*/
Texture* TextureGroup::ForName(const std::string& name)
{
	for (Texture* tex = first; tex; tex = tex->next)
	{
		if (name == tex->name)
			return tex;
	}
	return nullptr;
}

/*
==================
TextureGroup::ClearUsed
==================
*/
void TextureGroup::ClearUsed()
{
	for (Texture* tex = first; tex; tex = tex->next)
		tex->used = false;
}

/*
==================
TextureGroup::Add
==================
*/
void TextureGroup::Add(Texture* tx)
{
	numTextures++;

	// add sorted by name
	if (!first)
	{
		first = tx;
		return;
	}
	if (tx->name < first->name)
	{
		tx->next = first;
		first = tx;
		return;
	}

	Texture* qtex;
	qtex = first;
	while (qtex->next)
	{
		if (tx->name < qtex->next->name)
		{
			tx->next = qtex->next;
			qtex->next = tx;
			return;
		}
		qtex = qtex->next;
	}
	qtex->next = tx;
}

/*
==================
TextureGroup::FlushUnused
==================
*/
void TextureGroup::FlushUnused()
{
	Texture* head, * cur, * temp, * prev;
	head = cur = first;
	prev = nullptr;
	while (cur)
	{
		if (!cur->used)
		{
			temp = cur;
			if (head == cur)
				head = cur->next;
			if (prev)
				prev->next = cur->next;
			cur = cur->next;
			if (Textures::texMap[temp->name] == temp)	// don't nuke textures from other wads by the same name
				Textures::texMap[temp->name] = nullptr;
			delete temp;
			numTextures--;
		}
		else
		{
			prev = cur;
			cur = cur->next;
		}
	}
	first = head;
	flushed = true;
}

/*
TextureGroup::TextureGroup()
{
}


TextureGroup::~TextureGroup()
{
	for (TextureIterator texIt = texList.begin(); texIt != texList.end(); texIt++)
	{
		delete texIt->second;
	}
}



Texture* TextureGroup::operator[](const std::string &nameIn)
{
	if (Contains(nameIn)) return texList[nameIn];
	return nullptr;
}

bool TextureGroup::Contains(const std::string &nameIn)
{
	return (texList.count(nameIn) > 0);
}

void TextureGroup::Add(Texture *texIn)
{
//	texList[texIn->name] = texIn;
}
*/