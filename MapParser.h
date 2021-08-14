#pragma once
#include "Tokenizer.h"

class Brush;
class Entity;

enum class MapSchema { UNKNOWN, STANDARD, VALVE220 };

class MapParser
{
public:
	MapParser(const std::string_view data);
	~MapParser();

	void Read(Brush& blist, Entity& elist);
private:
	MapSchema schema;
	Tokenizer tokr;

	void ParseEntity(Brush& blist, Entity& elist);
	void ParseEPair(Entity& e);
	void ParseBrush(Brush& blist, Entity& e);
	void ParseFace(Brush& b);
};

