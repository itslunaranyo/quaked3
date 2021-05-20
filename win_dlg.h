//==============================
//	win_dlg.h
//==============================
#ifndef __WIN_DIALOGS_H__
#define __WIN_DIALOGS_H__

extern HWND g_hwndSetKeyvalsDlg;

bool DoColorSelect(vec3 &rgbOut);
bool DoColorSelect(const vec3 rgbIn, vec3 &rgbOut);
void DoFindBrush();
void DoRotate();
void DoSides(int nType);	// sikk - Brush Primitives (previously took no arguments)
void DoKeylist();
void DoMouselist();
void DoAbout();
void FillEntityListbox(HWND hwnd, bool bPointbased, bool bBrushbased);	// sikk - Create Entity Dialog
bool ConfirmClassnameHack(EntClass *desired);
void DoCreateEntity(bool bPointbased, bool bBrushbased, bool bSel, const vec3 origin);	// sikk - Create Entity Dialog
void DoMapInfo();	// sikk - Map Info Dialog
void DoEntityInfo();	// sikk - Entity Info Dialog
void DoScale();	// sikk - Brush Scaling Dialog
void DoCamSpeed();	// sikk - Camera Speed Dialog
void DoDefaultTexScale();	// sikk - Default Texture Scale Dialog
void DoFindKeyValue();	// sikk - Find Key/Value Dialog
void DoSetKeyValues();
bool ImportOptionsDialog(class CmdImportMap *cmdIM);

#endif	// __WIN_DIALOGS_H__