#pragma once

class Map;
class Entity;
class EPair;
class Brush;
class Face;

class MapWriter
{
public:
	MapWriter();
	~MapWriter();

	enum class BrushListFlag : int {
		NONE = 0,
		SELECTED = 1,
		ACTIVE = 2,
		REGIONED = 4
	};

	const static int WRITE_ALL = ((int)BrushListFlag::ACTIVE | (int)BrushListFlag::SELECTED | (int)BrushListFlag::REGIONED);
	const static int WRITE_VISIBLE = ((int)BrushListFlag::ACTIVE | (int)BrushListFlag::SELECTED);

	void WriteMap(Map& map, std::ostream& dest, const int subset = WRITE_ALL );


private:
	int writtenEnts, writtenBrushes, writtenEntBrushes;

	void WriteCompleteEntity(Entity& e, Map& map, std::ostream& out);
	void WritePartialEntity(Entity& e, Map& map, std::ostream& out, Brush* br, int count);
	void OpenEntity(Entity& e, std::ostream& out);
	void CloseEntity(std::ostream& out);
	void WriteEPair(EPair& ep, std::ostream& out);
	void WriteBrush(Brush& b, std::ostream& out);
	void WriteFace(Face& f, std::ostream& out);
};

