#ifndef __H_TEXTUREGROUP
#define __H_TEXTUREGROUP

#include <map>

class Texture;
typedef std::map<std::string, Texture*> TextureList;
typedef TextureList::iterator TextureIterator;

/*
// NEW SHIT I GUESS?
class TextureGroup
{
	friend class WadLoader;
public:
	TextureGroup();
	~TextureGroup();
	Texture* operator [](const std::string& nameIn);
	TextureIterator begin() { return texList.begin(); }
	TextureIterator end() { return texList.end(); }
	size_t size() { return texList.size(); }
	bool Contains(const std::string& nameIn);
	//bool Contains(Texture* tex);	// needed?

	std::string name;

private:
	void Add(Texture* texIn);
	TextureList texList;
	static Texture* nullTexture;
};
*/

class TextureGroup
{
public:
	TextureGroup();
	~TextureGroup();

	bool operator< (const TextureGroup& other) const { return name < other.name; }
	static bool sortcmp(const TextureGroup* a, const TextureGroup* b) { return *a < *b; }

	Texture* first;
	int	numTextures;
	int id;
	std::string name;
	bool flushed;

	Texture* ForName(const std::string& name);
	Texture* ByColor(const vec3 oc);
	void ClearUsed();
	void FlushUnused();
	void Flush();
	void Add(Texture* tx);
private:
};



#endif	// __H_TEXTUREGROUP