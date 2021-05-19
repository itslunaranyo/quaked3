//==============================
//	WndEntity.h
//==============================

#ifndef __WND_ENTITY_H__
#define __WND_ENTITY_H__

// win_ent.c ID's
#define ENT_CLASSLIST	0
#define ENT_COMMENT		1
#define ENT_CHECK1		2
#define ENT_CHECK2		3
#define ENT_CHECK3		4
#define ENT_CHECK4		5
#define ENT_CHECK5		6
#define ENT_CHECK6		7
#define ENT_CHECK7		8
#define ENT_CHECK8		9
#define ENT_CHECK9		10
#define ENT_CHECK10		11
#define ENT_CHECK11		12
#define ENT_CHECK12		13
#define ENT_PROPS		14
#define	ENT_DIR0		15
#define	ENT_DIR45		16
#define	ENT_DIR90		17
#define	ENT_DIR135		18
#define	ENT_DIR180		19
#define	ENT_DIR225		20
#define	ENT_DIR270		21
#define	ENT_DIR315		22
#define	ENT_DIRUP		23
#define	ENT_DIRDOWN		24
#define ENT_ADDPROP		25	// sikk - Entity Window Addition
#define ENT_DELPROP		26
#define ENT_CREATE		27	// sikk - Entity Window Addition
#define	ENT_KEYLABEL	28
#define	ENT_KEYFIELD	29
#define	ENT_VALUELABEL	30
#define	ENT_VALUEFIELD	31
#define ENT_COLOR		32
#define ENT_LAST		33


class WndEntity : public WndView
{
public:
	WndEntity();
	~WndEntity();

	//EntityView *ev;

	void Initialize();
	void ForceUpdate();
	bool TryCut();
	bool TryCopy();
	bool TryPaste();

	void FillClassList();
	int WindowProcedure(UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL CALLBACK EntityDlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL CALLBACK FieldProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL CALLBACK EntityListProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND w_hwndEntDialog;
	HWND w_hwndCtrls[ENT_LAST];

private:
	// lunaran - entity window now interacts through a dummy entity that acts as the union of 
	// all selected entities (for displaying mixed selections)
	// TODO: move this into a View class
	Entity	g_eEditEntity;
	char	g_nEditEntFlags[12];	// spawnflags in the entity inspector can be off/on/ambiguous
	char	g_szEditFlagNames[8][32];

	LRESULT(CALLBACK* DefaultFieldProc) (HWND, UINT, WPARAM, LPARAM);
	LRESULT(CALLBACK* DefaultEntityListProc) (HWND, UINT, WPARAM, LPARAM);
	void ResizeControls();
	void CreateControls();

	int OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void UpdateListSel();
	void UpdateUI();
	bool TryFieldCNP(int tryMsg);

	void ApplyAngle(int ang);
	void FlagChecked(int flag);
	void SetKeyValue(const char * key, const char * value);
	void SetKeyValue();
	void RemoveKeyValue();
	void EditKeyValue();
	void CreateEntity();
	void RefreshKeyValues();
	void FlagsFromEnt();

	void RefreshEditEntityFlags(int inFlags, bool first);
	void RefreshEditEntity();

};

#endif