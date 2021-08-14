#ifndef __ENTITYVIEW_H__
#define __ENTITYVIEW_H__
//==============================
//	EntityView.h
//==============================

#include "View.h"
#include "EPair.h"
#include "EntClass.h"

// entity inspector window interacts with mixed selections through a dummy entity 
// that acts as the union of all selected entities
class EntityView : public View
{
public:
	EntityView();
	~EntityView();

	void Refresh();

	const int GetFlagState(int f) { return (int)vFlags[f].state; }
	const std::string_view GetFlagName(int f) { return vFlags[f].name; }
	struct entEpair_t {
		entEpair_t() : mixed(false) {};
		EPair kv;
		bool mixed;
	};
	std::vector<entEpair_t>::iterator ePairsFirst() { return vPairs.begin(); }
	std::vector<entEpair_t>::iterator ePairsLast() { return vPairs.end(); }
	entEpair_t* FindKV(const std::string& key);

private:
	enum entFlagState_t {
		EFS_NO = BST_UNCHECKED,
		EFS_YES = BST_CHECKED,
		EFS_MIXED = BST_INDETERMINATE
	};
	struct entFlag_t {
		entFlag_t() : name(""), state(EFS_NO) {};
		std::string_view name;
		entFlagState_t state;
	};

	EntClass* vClass;
	bool vClassMixed;
	std::vector<entEpair_t> vPairs;
	entFlag_t vFlags[EntClass::MAX_FLAGS];
	const char* mixedkv = "__MIXED";	// this string should never become visible in QuakeEd

	entEpair_t* UnionKV(const std::string& key, const std::string& val);
	void DeleteKV(const std::string& key);

	void Reset();
	void UnionFlags(int inFlags, bool first);
	void UnionFlagNames(EntClass* ec);
};

#endif