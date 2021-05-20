//==============================
//	WndEntity.h
//==============================

#ifndef __WND_ENTITY_H__
#define __WND_ENTITY_H__

#include "Window.h"

class EntityView;

extern HWND g_hwndEntity;
extern WndEntity *g_wndEntity;

class WndEntity : public Window
{
public:
	WndEntity();
	~WndEntity();

	EntityView *ev;

	void Initialize();
	void SelectEntityColor();
	void ForceUpdate();
	bool TryCut();
	bool TryCopy();
	bool TryPaste();

	void FillClassList();
	int WindowProcedure(UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND w_hwndEntDialog;
	BOOL CALLBACK EntityDlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL CALLBACK FieldProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL CALLBACK EntityListProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	const char* mixedKVLabel = "[multiple values]";
	enum entWndID_t {
		ENT_CLASSLIST = 0,
		ENT_COMMENT = 1,
		ENT_CHECK1, ENT_CHECK2, ENT_CHECK3, ENT_CHECK4, ENT_CHECK5, ENT_CHECK6, ENT_CHECK7, ENT_CHECK8,
		ENT_CHECK9, ENT_CHECK10, ENT_CHECK11, ENT_CHECK12, ENT_CHECK13, ENT_CHECK14, ENT_CHECK15, ENT_CHECK16,
		ENT_DIR0, ENT_DIR45, ENT_DIR90, ENT_DIR135, ENT_DIR180, ENT_DIR225, ENT_DIR270, ENT_DIR315, ENT_DIRUP, ENT_DIRDOWN,
		ENT_PROPS, ENT_ADDPROP, ENT_DELPROP,
		ENT_CREATE,
		ENT_KEYLABEL, ENT_KEYFIELD, ENT_VALUELABEL, ENT_VALUEFIELD,
		ENT_COLOR,
		ENT_LAST
	};

	HWND w_hwndCtrls[ENT_LAST];

	LRESULT(CALLBACK* DefaultFieldProc) (HWND, UINT, WPARAM, LPARAM);
	LRESULT(CALLBACK* DefaultEntityListProc) (HWND, UINT, WPARAM, LPARAM);
	RECT WndEntRect(int left, int top, int right, int bottom);
	int OnResized();
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

};

#endif