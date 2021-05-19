#ifndef __H_TEXTUREGROUP
#define __H_TEXTUREGROUP

#include <map>

class Texture;
typedef std::map<std::string, Texture*> TextureList;
typedef TextureList::iterator TextureIterator;

class TextureGroup
{
	friend class WadLoader;
public:
	TextureGroup();
	~TextureGroup();
	Texture* operator [](const std::string* nameIn);
	TextureIterator begin() { return texList.begin(); }
	TextureIterator end() { return texList.end(); }
	size_t size() { return texList.size(); }
	bool Contains(const std::string* nameIn);
	//bool Contains(Texture* tex);	// needed?

	std::string name;

private:
	void Add(Texture* texIn);
	TextureList texList;
	static Texture* nullTexture;
};

#endif	// __H_TEXTUREGROUP