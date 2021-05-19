//==============================
//	Tool.cpp
//==============================

#include "qe3.h"

/*
============================================================

TOOLS

- the existence of the tool object should be all that is necessary to be
in that tool's "mode". a tool instance should be safe to delete from anywhere
and not leave any residual editor state associated with its use.

- a tool returns 1 if the input was handled, 0 if it was ignored (note that
this is the opposite of the windows paradigm, where an input handler returns
true if the input *wasn't* handled and should continue to be processed)

- tools should not capture any input they don't use, to ensure lower tools
aren't starved for their expected inputs. this especially includes unused
shift/ctrl/alt modifier combos. as a rule, every tool should return false on
any mousemove or mouseup if the tool isn't hot, regardless of side effects.

============================================================
*/

Tool::Tool(const char* n, bool isModal = false) : name(n), modal(isModal), hot(false)
{
	// only one modal tool is allowed at a time
	if (modal && g_qeglobals.d_tools.size() && g_qeglobals.d_tools.back()->modal)
		delete g_qeglobals.d_tools.back();

	g_qeglobals.d_tools.push_back(this);
}

Tool::~Tool()
{
	// enforce tools removed in reverse order
	assert(g_qeglobals.d_tools.back() == this);

	if (hot) ReleaseCapture();	// jic

	g_qeglobals.d_tools.pop_back();
}

bool Tool::Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd) { return hot; }
bool Tool::Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd) { return hot; }
bool Tool::Input1D(UINT uMsg, WPARAM wParam, LPARAM lParam, ZView &v, WndView &vWnd) { return hot; }
bool Tool::InputTex(UINT uMsg, WPARAM wParam, LPARAM lParam, TextureView &v, WndView &vWnd) { return hot; }
bool Tool::Input(UINT uMsg, WPARAM wParam, LPARAM lParam) { return hot; }


int Tool::HandleInput3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndView &vWnd)
{
	if (!FilterInput(uMsg)) return 0;
	int out;
	Tool* ht = HotTool();
	if (ht)
	{
		//if (uMsg == WM_LBUTTONDOWN) Sys_Printf("sending HOT lbtndn input to %s\n", ht->name);
		//if (uMsg == WM_LBUTTONUP) Sys_Printf("sending HOT lbtnup input to %s\n", ht->name);
		return ht->Input3D(uMsg, wParam, lParam, v, vWnd);
	}

	for (auto rtIt = g_qeglobals.d_tools.rbegin(); rtIt != g_qeglobals.d_tools.rend(); ++rtIt)
	{
		//if (uMsg == WM_LBUTTONDOWN) Sys_Printf("sending lbtndn input to %s\n", (*rtIt)->name);
		//if (uMsg == WM_LBUTTONUP) Sys_Printf("sending lbtnup input to %s\n", (*rtIt)->name);
		out = (*rtIt)->Input3D(uMsg, wParam, lParam, v, vWnd);
		if (out)
		{
			//if (uMsg == WM_LBUTTONDOWN) Sys_Printf("  it says it handled it\n");
			return out;
		}
	}

	return 0;
}

int Tool::HandleInput2D(UINT uMsg, WPARAM wParam, LPARAM lParam, XYZView &v, WndView &vWnd)
{
	if (!FilterInput(uMsg)) return 0;

	int out;
	Tool* ht = HotTool();
	if (ht)
		return ht->Input2D(uMsg, wParam, lParam, v, vWnd);

	for (auto rtIt = g_qeglobals.d_tools.rbegin(); rtIt != g_qeglobals.d_tools.rend(); ++rtIt)
	{
		out = (*rtIt)->Input2D(uMsg, wParam, lParam, v, vWnd);
		if (out) return out;
	}

	return 0;
}

int Tool::HandleInput1D(UINT uMsg, WPARAM wParam, LPARAM lParam, ZView &v, WndView &vWnd)
{
	if (!FilterInput(uMsg)) return 0;

	int out;
	Tool* ht = HotTool();
	if (ht)
		return ht->Input1D(uMsg, wParam, lParam, v, vWnd);

	for (auto rtIt = g_qeglobals.d_tools.rbegin(); rtIt != g_qeglobals.d_tools.rend(); ++rtIt)
	{
		out = (*rtIt)->Input1D(uMsg, wParam, lParam, v, vWnd);
		if (out) return out;
	}

	return 0;
}

int Tool::HandleInputTex(UINT uMsg, WPARAM wParam, LPARAM lParam, TextureView &v, WndView &vWnd)
{
	if (!FilterInput(uMsg)) return 0;

	int out;
	Tool* ht = HotTool();
	if (ht)
		return ht->InputTex(uMsg, wParam, lParam, v, vWnd);

	for (auto rtIt = g_qeglobals.d_tools.rbegin(); rtIt != g_qeglobals.d_tools.rend(); ++rtIt)
	{
		out = (*rtIt)->InputTex(uMsg, wParam, lParam, v, vWnd);
		if (out) return out;
	}

	return 0;
}

int Tool::HandleInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!FilterInput(uMsg)) return 0;

	int out;
	Tool* ht = HotTool();
	if (ht)
		return ht->Input(uMsg, wParam, lParam);

	for (auto rtIt = g_qeglobals.d_tools.rbegin(); rtIt != g_qeglobals.d_tools.rend(); ++rtIt)
	{
		out = (*rtIt)->Input(uMsg, wParam, lParam);
		if (out) return out;
	}

	return 0;
}

Tool* Tool::HotTool()
{
	for (auto rtIt = g_qeglobals.d_tools.rbegin(); rtIt != g_qeglobals.d_tools.rend(); ++rtIt)
		if ((*rtIt)->hot)
			return (*rtIt);
	return nullptr;
}

bool Tool::FilterInput(UINT uMsg)
{
	return ( uMsg == WM_COMMAND ||
			(uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST) ||
			(uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) );
}
