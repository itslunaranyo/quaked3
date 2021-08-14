#pragma once

class EntClass;

class EntClassInitializer
{
public:
	EntClassInitializer();
	~EntClassInitializer();

	bool InitForProject();
private:
	void FinalizeEntClass(EntClass& ec);
	//EntClass* InitFromText(char* text);
};

