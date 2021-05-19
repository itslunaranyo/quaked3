//==============================
//	WndEntity.h
//==============================

#ifndef __WND_ENTITY_H__
#define __WND_ENTITY_H__


class WndEntity : public WndView
{
public:
	WndEntity();
	~WndEntity();

	//EntityView *ev;

	void Initialize();
	void ForceUpdate();

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