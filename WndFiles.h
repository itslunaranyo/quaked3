//==============================
//	WndFiles.h
//==============================

#ifndef __WND_FILES_H__
#define __WND_FILES_H__

bool Dlg_FileOpen(std::string& sel, const char* _dir, const char* _filter, const char* _title, DWORD flags);
bool Dlg_FileSave(std::string& sel, const char* _dir, const char* _filter, const char* _title, DWORD flags);

void	Dlg_MapOpen();
bool	Dlg_MapSaveAs();
bool	ConfirmModified();
void	Dlg_MapImport();
void	Dlg_MapExport();

#endif