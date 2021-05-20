//==============================
//	Tool.cpp
//==============================

#include "pre.h"
#include "qe3.h"
#include "Tool.h"
#include "Window.h"

/*
================================================================

TOOLS

- tools translate user input into commands that modify the scene. they can
build the commands up front, on completion, actively reconfigure them, and
in some cases modify them after completion.

- multiple tools exist at once on a stack, receiving input in descending
order until one of them reports that it handled it. tools can block input
to lower tools by falsely reporting, which is mainly exploited during mouse
drags (when a tool is "hot") to enforce one-command-at-a-time editing and
ensure that the context a command was created in doesn't change before its
completion.

- a tool returns true if an input was handled, false if it was ignored. note
that this is the opposite of the windows paradigm, where an input returns
true if the input *wasn't* handled and should continue to be processed.

- tools should otherwise not capture any input they don't use, to ensure 
lower tools aren't starved for their expected inputs. this especially 
includes unused shift/ctrl/alt modifier combos. as a rule, every tool should
return false on any mousemove or mouseup if the tool isn't hot, regardless
of side effects.

- the existence of the tool object should be all that is necessary to be in
that tool's "mode". a tool instance should be safe to delete from anywhere
and not leave any residual editor state associated with its use. 

- 'modal' tools are mutually exclusive with each other, and represent
entering an editing mode where the normal set of inputs doesn't apply (for
example, clicking and dragging in the camera view moves the selection or
drag planes, but in clip mode it places and moves clip points instead.)

================================================================
*/

std::vector<Tool*> Tool::stack;

Tool::Tool(const char* n, bool isModal = false) : name(n), modal(isModal), hot(false)
{
	// only one modal tool is allowed at a time
	if (modal && stack.size() && stack.back()->modal)
		delete stack.back();

	stack.push_back(this);
	WndMain_UpdateGridStatusBar();
}

Tool::~Tool()
{
	// enforce tools removed in reverse order
	assert(stack.back() == this);

	if (hot) ReleaseCapture();	// jic

	stack.pop_back();
	WndMain_UpdateGridStatusBar();
}

bool Tool::Input3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndCamera &vWnd) { return hot; }
bool Tool::Input2D(UINT uMsg, WPARAM wParam, LPARAM lParam, GridView &v, WndGrid &vWnd) { return hot; }
bool Tool::Input1D(UINT uMsg, WPARAM wParam, LPARAM lParam, ZView &v, WndZChecker &vWnd) { return hot; }
bool Tool::InputTex(UINT uMsg, WPARAM wParam, LPARAM lParam, TextureView &v, WndTexture &vWnd) { return hot; }
bool Tool::Input(UINT uMsg, WPARAM wParam, LPARAM lParam) { return hot; }


int Tool::HandleInput3D(UINT uMsg, WPARAM wParam, LPARAM lParam, CameraView &v, WndCamera &vWnd)
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

	for (auto rtIt = stack.rbegin(); rtIt != stack.rend(); ++rtIt)
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

int Tool::HandleInput2D(UINT uMsg, WPARAM wParam, LPARAM lParam, GridView &v, WndGrid &vWnd)
{
	if (!FilterInput(uMsg)) return 0;

	int out;
	Tool* ht = HotTool();
	if (ht)
		return ht->Input2D(uMsg, wParam, lParam, v, vWnd);

	for (auto rtIt = stack.rbegin(); rtIt != stack.rend(); ++rtIt)
	{
		out = (*rtIt)->Input2D(uMsg, wParam, lParam, v, vWnd);
		if (out) return out;
	}

	return 0;
}

int Tool::HandleInput1D(UINT uMsg, WPARAM wParam, LPARAM lParam, ZView &v, WndZChecker &vWnd)
{
	if (!FilterInput(uMsg)) return 0;

	int out;
	Tool* ht = HotTool();
	if (ht)
		return ht->Input1D(uMsg, wParam, lParam, v, vWnd);

	for (auto rtIt = stack.rbegin(); rtIt != stack.rend(); ++rtIt)
	{
		out = (*rtIt)->Input1D(uMsg, wParam, lParam, v, vWnd);
		if (out) return out;
	}

	return 0;
}

int Tool::HandleInputTex(UINT uMsg, WPARAM wParam, LPARAM lParam, TextureView &v, WndTexture &vWnd)
{
	if (!FilterInput(uMsg)) return 0;

	int out;
	Tool* ht = HotTool();
	if (ht)
		return ht->InputTex(uMsg, wParam, lParam, v, vWnd);

	for (auto rtIt = stack.rbegin(); rtIt != stack.rend(); ++rtIt)
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

	for (auto rtIt = stack.rbegin(); rtIt != stack.rend(); ++rtIt)
	{
		out = (*rtIt)->Input(uMsg, wParam, lParam);
		if (out) return out;
	}

	return 0;
}

Tool* Tool::ModalTool()
{
	if (stack.empty())
		return nullptr;
	if (stack.back()->modal)
		return stack.back();
	return nullptr;
}

Tool* Tool::HotTool()
{
	for (auto rtIt = stack.rbegin(); rtIt != stack.rend(); ++rtIt)
		if ((*rtIt)->hot)
			return (*rtIt);
	return nullptr;
}

bool Tool::FilterInput(UINT uMsg)
{
	return ( uMsg == WM_COMMAND || uMsg == WM_REALTIME ||
			(uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST) ||
			(uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) || // all mouse messages are between MOUSEFIRST and MOUSELAST
			(uMsg == WM_MOUSELEAVE) );	// "syke no they're not lol" -windows
}

/*
==================
Tool::Crosshair
==================
*/
void Tool::Crosshair(bool bCrossHair)
{
	SetCursor((bCrossHair) ? LoadCursor(NULL, IDC_CROSS) : LoadCursor(NULL, IDC_ARROW));
}

// FIXME: https://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-getmessagepos
void Tool::MsgToXY(const LPARAM lParam, const Window &wnd, int &xPos, int &yPos)
{
	xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
	yPos = (short)HIWORD(lParam);  // vertical position of cursor 
	yPos = (int)wnd.clientRect.bottom - 1 - yPos;
}

