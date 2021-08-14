#pragma once

#include "palette.h"

class PaletteReader
{
public:
	PaletteReader() {};
	~PaletteReader() {};

	bool Load(const std::string& f);
    void LoadDefault();
	void GenerateErrorPalette();

private:
	void GenerateGammaTable(byte* table);
};

