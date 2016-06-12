#include "Dialog/MGAFinder.hpp"

#include "resource.h"

#include <Orbitersdk.h>

#include <windows.h>
#include <iostream>

#include "DlgCtrl.h"
#include <stdio.h>
#include <io.h>

#include "orbiterkep/proto/solution.pb-c.h"
#include "orbiterkep/proto/parameters.pb-c.h"

#include "orbiterkep/opt/optimise-c.h"
#include "orbiterkep/proto/ext-c.h"

extern MGAFinder * g_mgaFinder;
extern Optimization * g_optimizer;

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

	g_optimizer->RunOptimization(hDlg);
}

MGAFinder::~MGAFinder() {
	oapiUnregisterCustomCmd(dwCmd);
}

void MGAFinder::InitDialog(HWND _hDlg)
{
	hDlg = _hDlg;

	FillCBodyList(hDlg);
	FillProblemList(hDlg);

	MGAFinder::ParamToUI(g_optimizer->param());

	g_optimizer->Update(_hDlg);
}

void MGAFinder::OpenDialog()
{
	hDlg = oapiOpenDialogEx(hInst, IDD_MGA_FINDER, MGAFinderProc, 
		DLG_CAPTIONCLOSE | DLG_CAPTIONHELP, this);
}
	
void MGAFinder::CloseDialog()
{
	if (hDlg) {
		oapiCloseDialog(hDlg);
		hDlg = NULL;
	}
}

void MGAFinder::UIToParam(Orbiterkep__Parameters &param) {
	char buf[256];
	// Launch date
	GetDlgItemText(hDlg, IDC_T0_MIN, buf, 256);
	sscanf_s(buf, "%lf", &(param.t0->lb));
	GetDlgItemText(hDlg, IDC_T0_MAX, buf, 256);
	sscanf_s(buf, "%lf", &(param.t0->ub));
	// Time of flight
	GetDlgItemText(hDlg, IDC_TOF_MIN, buf, 256);
	sscanf_s(buf, "%lf", &(param.tof->lb));
	GetDlgItemText(hDlg, IDC_TOF_MAX, buf, 256);
	sscanf_s(buf, "%lf", &(param.tof->ub));
	// Ejection VInf
	GetDlgItemText(hDlg, IDC_VINF_MIN, buf, 256);
	sscanf_s(buf, "%lf", &(param.vinf->lb));
	GetDlgItemText(hDlg, IDC_VINF_MAX, buf, 256);
	sscanf_s(buf, "%lf", &(param.vinf->ub));

	param.add_arr_vinf = SendDlgItemMessage(hDlg, IDC_ADD_ARR_VINF, BM_GETCHECK, 0, 0);
	param.add_dep_vinf = SendDlgItemMessage(hDlg, IDC_ADD_DEP_VINF, BM_GETCHECK, 0, 0);
	param.circularize = SendDlgItemMessage(hDlg, IDC_CIRCULARIZE, BM_GETCHECK, 0, 0);

	GetDlgItemText(hDlg, IDC_EDIT_N_ISL, buf, 256);
	sscanf_s(buf, "%d", &(param.pagmo->n_isl));
	GetDlgItemText(hDlg, IDC_EDIT_POP, buf, 256);
	sscanf_s(buf, "%d", &(param.pagmo->population));
	GetDlgItemText(hDlg, IDC_EDIT_N_GEN, buf, 256);
	sscanf_s(buf, "%d", &(param.pagmo->n_gen));
	GetDlgItemText(hDlg, IDC_EDIT_MF, buf, 256);
	sscanf_s(buf, "%d", &(param.pagmo->mf));
	GetDlgItemText(hDlg, IDC_EDIT_MR, buf, 256);
	sscanf_s(buf, "%lf", &(param.pagmo->mr));

	int nPlanets = SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_GETCOUNT, 0, 0);
	param.n_planets = nPlanets;
	for (int i = 0; i < nPlanets; ++i) {
		SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_GETTEXT, i, (LPARAM)buf);
		strcpy(param.planets[i], buf);
	}
}

void MGAFinder::ParamToUI(const Orbiterkep__Parameters &param)
{
	char buf[256];
	// Launch date
	sprintf_s(buf, "%0.6lf", param.t0->lb);
	SetDlgItemText(hDlg, IDC_T0_MIN, buf);
	sprintf_s(buf, "%0.6lf", param.t0->ub);
	SetDlgItemText(hDlg, IDC_T0_MAX, buf);
	// Time of Flight
	sprintf_s(buf, "%0.2lf", param.tof->lb);
	SetDlgItemText(hDlg, IDC_TOF_MIN, buf);
	sprintf_s(buf, "%0.2lf", param.tof->ub);
	SetDlgItemText(hDlg, IDC_TOF_MAX, buf);
	// Ejection VInf
	sprintf_s(buf, "%0.3lf", param.vinf->lb);
	SetDlgItemText(hDlg, IDC_VINF_MIN, buf);
	sprintf_s(buf, "%0.3lf", param.vinf->ub);
	SetDlgItemText(hDlg, IDC_VINF_MAX, buf);

	SendDlgItemMessage(hDlg, IDC_ADD_ARR_VINF, BM_SETCHECK, param.add_arr_vinf, 0);
	SendDlgItemMessage(hDlg, IDC_ADD_DEP_VINF, BM_SETCHECK, param.add_dep_vinf, 0);
	SendDlgItemMessage(hDlg, IDC_CIRCULARIZE, BM_SETCHECK, param.circularize, 0);
	
	sprintf_s(buf, "%d", param.pagmo->n_isl);
	SetDlgItemText(hDlg, IDC_EDIT_N_ISL, buf);
	sprintf_s(buf, "%d", param.pagmo->population);
	SetDlgItemText(hDlg, IDC_EDIT_POP, buf);
	sprintf_s(buf, "%d", param.pagmo->n_gen);
	SetDlgItemText(hDlg, IDC_EDIT_N_GEN, buf);
	sprintf_s(buf, "%d", param.pagmo->mf);
	SetDlgItemText(hDlg, IDC_EDIT_MF, buf);
	sprintf_s(buf, "%0.3lf", param.pagmo->mr);
	SetDlgItemText(hDlg, IDC_EDIT_MR, buf);

	SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_RESETCONTENT, 0, 0);
	if (param.n_planets > 0) {
		for (unsigned int i = 0; i < param.n_planets; ++i) {
			char * planet_name = param.planets[i];
			SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_ADDSTRING, 0, (LPARAM)planet_name);
		}
	}

	SendDlgItemMessage(hDlg, IDC_PROBLEM, CB_SELECTSTRING, -1, (LPARAM)param.problem);

	if (g_optimizer->has_solution()) {
		SetDlgItemText(hDlg, IDC_SOLUTION, g_optimizer->get_solution_str().c_str());
	}

	if (g_optimizer->computing()) {
		SetDlgItemText(hDlg, ID_OPT, "Cancel");
	} else {
		SetDlgItemText(hDlg, ID_OPT, "Find Optimum");
	}
}

void MGAFinder::FillCBodyList(HWND hDlg)
{
	int hList = IDC_PLANET_SELECT;

	char cbuf[256];
	SendDlgItemMessage(hDlg, hList, CB_RESETCONTENT, 0, 0);
	for (DWORD n = 0; n < oapiGetGbodyCount(); ++n) {
		int type = oapiGetObjectType(oapiGetGbodyByIndex(n));
		if (type == OBJTP_PLANET) {
			oapiGetObjectName(oapiGetGbodyByIndex(n), cbuf, 256);
			SendDlgItemMessage(hDlg, hList, CB_ADDSTRING, 0, (LPARAM)cbuf);
		}
	}
}

void MGAFinder::FillProblemList(HWND hDlg)
{
	int hList = IDC_PROBLEM;

	SendDlgItemMessage(hDlg, hList, CB_RESETCONTENT, 0, 0);
	SendDlgItemMessage(hDlg, hList, CB_ADDSTRING, 0, (LPARAM)"MGA");
	SendDlgItemMessage(hDlg, hList, CB_ADDSTRING, 0, (LPARAM)"MGA-1DSM");
}


void MGAFinder::SolutionToUI(const Orbiterkep__TransXSolution &sol) {
	SetDlgItemText(hDlg, IDC_SOLUTION, g_optimizer->get_solution_str().c_str());
}

void MGAFinder::PlanetList_AddItem(char * item) {
	PlanetList_InsertItemAtIndex(item, -1);
}

void MGAFinder::PlanetList_RemoveItem(char * item) {
	int idx = SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_FINDSTRING, -1, (LPARAM)item);
	if (idx != LB_ERR) {
		PlanetList_RemoveItemAtIndex(idx);
	}
}

void MGAFinder::PlanetList_RemoveItemAtIndex(int idx) {
	SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_DELETESTRING, idx, 0);
}

void MGAFinder::PlanetList_InsertItemAtIndex(char * item, int idx) {
	SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_INSERTSTRING, idx, (LPARAM)item);
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
		case ID_OPT: {
			UIToParam(g_optimizer->param());
			if (g_optimizer->computing()) {
				g_optimizer->Cancel();
				SetDlgItemText(hDlg, ID_OPT, "Find Optimum");
			} else {
				g_optimizer->RunOptimization(hDlg);
				SetDlgItemText(hDlg, ID_OPT, "Cancel");
			}
			break;
		}
		case IDC_PLANET_ADD: {
			char str[200];
			int sel = SendDlgItemMessage(hDlg, IDC_PLANET_SELECT, CB_GETCURSEL, 0, 0);
			if (sel != CB_ERR) {
				SendDlgItemMessage(hDlg, IDC_PLANET_SELECT, CB_GETLBTEXT, sel, (LPARAM)str);
				PlanetList_AddItem(str);
			}
		}
			break;
		case IDC_PLANET_DEL: {
			int sel = SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_GETCURSEL, 0, 0);
			if (sel != LB_ERR) {
				PlanetList_RemoveItemAtIndex(sel);
			}
		}
			break;
		case IDHELP:
			return TRUE;
		}
		break;
	case WM_LBUTTONUP:
		break;
	case WM_LBUTTONDOWN:
		break;
	case WM_OPTIMIZATION_READY:
		SolutionToUI(g_optimizer->get_best_solution());
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