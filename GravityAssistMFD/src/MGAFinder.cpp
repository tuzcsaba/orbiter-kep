#include "MGAFinder.h"

#include "resource.h"

#include "orbiterkep/opt/optimise-c.h"

#include <Orbitersdk.h>

#include "DlgCtrl.h"
#include <stdio.h>
#include <io.h>

extern MGAFinder *g_mgaFinder;

void OpenDialog(void * context);
BOOL CALLBACK MGAFinderProc(HWND, UINT, WPARAM, LPARAM);

MGAFinder::MGAFinder(HINSTANCE hDLL)
{
	hInst = hDLL;
	hDlg = NULL;

	dwCmd = oapiRegisterCustomCmd(
		"Find Optimal MGA Solution",
		"Define and execute Multiple Gravity Assist optimization task",
		::OpenDialog, this);
}

MGAFinder::~MGAFinder() {
	CloseDialog();
	oapiUnregisterCustomCmd(dwCmd);
}

void MGAFinder::InitDialog(HWND _hDlg)
{
	hDlg = _hDlg;	
}

void MGAFinder::OpenDialog()
{
	hDlg = oapiOpenDialogEx(hInst, IDD_MGAFINDER, MGAFinderProc, 
		DLG_CAPTIONCLOSE | DLG_CAPTIONHELP, this);

	_Orbiterkep__Parameters  param = ORBITERKEP__PARAMETERS__INIT;
	param.n_planets = 3;
	char* planets[] = { 
		"earth", "venus", "mercury" 
	};
	param.planets = planets;
	param.n_single_objective_algos = 1;
	char* single_objective_algos[] = { "jde" };
	param.single_objective_algos = single_objective_algos;

	_Orbiterkep__TransXSolution * sol = orbiterkep_optimize(param);

	orbiterkep__trans_xsolution__free_unpacked(sol, NULL);
}

void MGAFinder::CloseDialog()
{
	if (hDlg) {
		oapiCloseDialog(hDlg);
		hDlg = NULL;
	}
}

void MGAFinder::FillCBodyList(HWND hDlg)
{
	int hList = IDC_LST_PLANETS;

	char cbuf[256];
	SendDlgItemMessage(hDlg, hList, CB_RESETCONTENT, 0, 0);
	for (DWORD n = 0; n < oapiGetGbodyCount(); ++n) {
		oapiGetObjectName(oapiGetGbodyByIndex(n), cbuf, 256);
		SendDlgItemMessage(hDlg, hList, CB_ADDSTRING, 0, (LPARAM)cbuf);
	}
}

int MGAFinder::MsgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		InitDialog(hDlg);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			CloseDialog();
			return TRUE;
		case IDHELP:
			return TRUE;
		}
		break;
	}
	return oapiDefDialogProc(hDlg, uMsg, wParam, lParam);
}

// ===========================================================================
// Non-member functions

void OpenDialog(void *context)
{
	((MGAFinder*)context)->OpenDialog();
}

BOOL CALLBACK MGAFinderProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return g_mgaFinder->MsgProc(hDlg, uMsg, wParam, lParam);
}