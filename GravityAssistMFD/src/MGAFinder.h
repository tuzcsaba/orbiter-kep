#ifndef __MGAFinder_H
#define __MGAFinder_H

#include <Windows.h>
#include <CommCtrl.h>

class MGAFinder {
public:
	MGAFinder(HINSTANCE hDLL);
	~MGAFinder();
	void OpenDialog();
	void CloseDialog();
	void InitDialog(HWND hDlg);

	int MsgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	HWND DlgHandle() const { return hDlg; }
	HINSTANCE InstHandle() const { return hInst; }

	void FillCBodyList(HWND hDlg);

private:
	DWORD dwCmd;
	HWND hDlg;
	HINSTANCE hInst;	
};

#endif