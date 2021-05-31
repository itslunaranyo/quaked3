//==============================
//	CmdImportMap.h
//==============================

#ifndef __COMMAND_IMPORT_MAP_H__
#define __COMMAND_IMPORT_MAP_H__

#include "qe3.h"
#include "Command.h"
#include "CmdAddRemove.h"
#include "CmdSetKeyvalue.h"

class CmdImportMap : public Command
{
public:
	CmdImportMap();
	~CmdImportMap() {}

	enum worldKVMerge
	{
		KVM_IGNORE,
		KVM_ADD,
		KVM_OVERWRITE
	};

	void MergeWads(bool merge);
	void AdjustTargets(bool inc);
	void MergeWorldKeys(worldKVMerge merge);
	void File(const char* fname);

	int BrushDelta() { return cmdAR.BrushDelta(); };
	int EntityDelta() { return cmdAR.EntityDelta(); };

private:
	CmdAddRemove cmdAR;
	std::vector<CmdSetKeyvalue*> cmdWorldKVs;
	qeBuffer filename;
	bool mergewads;
	bool adjusttargets;
	worldKVMerge mergekvs;

	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Sel_Impl();

};

#endif	// __COMMAND_IMPORT_MAP_H__
