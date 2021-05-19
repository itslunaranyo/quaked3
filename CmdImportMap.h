//==============================
//	CmdImportMap.h
//==============================

#ifndef __COMMAND_IMPORT_MAP_H__
#define __COMMAND_IMPORT_MAP_H__

class CmdImportMap : public Command
{
public:
	CmdImportMap();
	~CmdImportMap() {}

	void File(const char* fname);

private:
	CmdAddRemove cmdAR;
	qeBuffer filename;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_IMPORT_MAP_H__
