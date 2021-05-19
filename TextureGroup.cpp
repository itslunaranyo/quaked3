#include "qe3.h"
#include "TextureGroup.h"
//#include "Texture.h"



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



Texture* TextureGroup::operator[](const std::string *nameIn)
{
	if (Contains(nameIn)) return texList[nameIn];
	return nullptr;
}

bool TextureGroup::Contains(const std::string *nameIn)
{
	return (texList.count(*nameIn) > 0);
}

void TextureGroup::Add(Texture *texIn)
{
//	texList[texIn->name] = texIn;
}
