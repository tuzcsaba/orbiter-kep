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

	g_optimizer->Update(hDlg);
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

void MGAFinder::UIToParam() {
	char buf[256];

	Orbiterkep__Parameters * newParam = (Orbiterkep__Parameters *)malloc(sizeof(Orbiterkep__Parameters));
	orbiterkep__parameters__init(newParam);
	newParam->t0 = (Orbiterkep__ParamBounds *)malloc(sizeof(Orbiterkep__ParamBounds));
	orbiterkep__param_bounds__init(newParam->t0);
	newParam->t0->has_lb = 1; newParam->t0->has_ub = 1;
	newParam->tof = (Orbiterkep__ParamBounds *)malloc(sizeof(Orbiterkep__ParamBounds));
	orbiterkep__param_bounds__init(newParam->tof);
	newParam->tof->has_lb = 1; newParam->tof->has_ub = 1;
	newParam->vinf = (Orbiterkep__ParamBounds *)malloc(sizeof(Orbiterkep__ParamBounds));
	orbiterkep__param_bounds__init(newParam->vinf);
	newParam->vinf->has_lb = 1; newParam->vinf->has_ub = 1;
	newParam->pagmo = (Orbiterkep__ParamPaGMO *)malloc(sizeof(Orbiterkep__ParamPaGMO));
	orbiterkep__param_pa_gmo__init(newParam->pagmo);
	// Launch date
	GetDlgItemText(hDlg, IDC_T0_MIN, buf, 255);
	sscanf_s(buf, "%lf", &(newParam->t0->lb));
	GetDlgItemText(hDlg, IDC_T0_MAX, buf, 255);
	sscanf_s(buf, "%lf", &(newParam->t0->ub));
	// Time of flight
	GetDlgItemText(hDlg, IDC_TOF_MIN, buf, 255);
	sscanf_s(buf, "%lf", &(newParam->tof->lb));
	GetDlgItemText(hDlg, IDC_TOF_MAX, buf, 255);
	sscanf_s(buf, "%lf", &(newParam->tof->ub));
	// Ejection VInf
	GetDlgItemText(hDlg, IDC_VINF_MIN, buf, 255);
	sscanf_s(buf, "%lf", &(newParam->vinf->lb));
	GetDlgItemText(hDlg, IDC_VINF_MAX, buf, 255);
	sscanf_s(buf, "%lf", &(newParam->vinf->ub));

	newParam->add_arr_vinf = SendDlgItemMessage(hDlg, IDC_ADD_ARR_VINF, BM_GETCHECK, 0, 0);
	newParam->has_add_arr_vinf = 1;
	newParam->add_dep_vinf = SendDlgItemMessage(hDlg, IDC_ADD_DEP_VINF, BM_GETCHECK, 0, 0);
	newParam->has_add_dep_vinf = 1;
	newParam->circularize = SendDlgItemMessage(hDlg, IDC_CIRCULARIZE, BM_GETCHECK, 0, 0);
	newParam->has_circularize = 1;

	GetDlgItemText(hDlg, IDC_EDIT_N_ISL, buf, 255);
	sscanf_s(buf, "%d", &(newParam->pagmo->n_isl));
	newParam->pagmo->has_n_isl = 1;
	GetDlgItemText(hDlg, IDC_EDIT_POP, buf, 255);
	sscanf_s(buf, "%d", &(newParam->pagmo->population));
	newParam->pagmo->has_population = 1;
	GetDlgItemText(hDlg, IDC_EDIT_N_GEN, buf, 255);
	sscanf_s(buf, "%d", &(newParam->pagmo->n_gen));
	newParam->pagmo->has_n_gen = 1;
	GetDlgItemText(hDlg, IDC_EDIT_MF, buf, 255);
	sscanf_s(buf, "%d", &(newParam->pagmo->mf));
	newParam->pagmo->has_mf = 1;
	GetDlgItemText(hDlg, IDC_EDIT_MR, buf, 255);
	sscanf_s(buf, "%lf", &(newParam->pagmo->mr));
	newParam->pagmo->has_mr = 1;
	GetDlgItemText(hDlg, IDC_DEP_ALT, buf, 255);
	sscanf_s(buf, "%lf", &(newParam->dep_altitude));
	newParam->has_dep_altitude = 1;
	GetDlgItemText(hDlg, IDC_ARR_ALT, buf, 255);
	sscanf_s(buf, "%lf", &(newParam->arr_altitude));
	newParam->has_arr_altitude = 1;

	char * problem_buf = (char *)malloc(sizeof(char) * 20);
	GetDlgItemText(hDlg, IDC_PROBLEM, problem_buf, 20);
	newParam->problem = problem_buf;

	int nPlanets = SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_GETCOUNT, 0, 0);
	newParam->planets = (char **)malloc(sizeof(char *) * nPlanets);
	newParam->n_planets = nPlanets;
	for (int i = 0; i < nPlanets; ++i) {
		SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_GETTEXT, i, (LPARAM)buf);
		newParam->planets[i] = (char *)malloc(sizeof(char) * (strlen(buf) + 1));
		strcpy(newParam->planets[i], buf);
	}

	char ** single_obj_algos = (char **)malloc(sizeof(char *));
	single_obj_algos[0] = (char *)malloc(sizeof(char) * 4);
	strcpy(single_obj_algos[0], "jde");
	newParam->n_single_objective_algos = 1;
	newParam->single_objective_algos = single_obj_algos;

	char ** multi_obj_algos = (char **)malloc(sizeof(char *));
	multi_obj_algos[0] = (char *)malloc(sizeof(char) * 6);
	strcpy(multi_obj_algos[0], "nsga2");
	newParam->n_multi_objective_algos = 1;
	newParam->multi_objective_algos = multi_obj_algos;

	newParam->has_n_trials = 1;
	newParam->n_trials = 1;

	newParam->has_max_deltav = 1;
	newParam->max_deltav = 24000;

	newParam->has_dep_altitude = 1;
	newParam->dep_altitude = 300;

	newParam->has_arr_altitude = 1;
	newParam->arr_altitude = 300;

	newParam->has_multi_objective = 1;
	newParam->multi_objective = 0;

	newParam->has_use_db = 1;
	newParam->use_db = 0;

	newParam->has_use_spice = 1;
	newParam->use_spice = 0;

	g_optimizer->update_parameters(newParam, false);
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
	sprintf_s(buf, "%0.3lf", param.dep_altitude);
	SetDlgItemText(hDlg, IDC_DEP_ALT, buf);
	sprintf_s(buf, "%0.3lf", param.arr_altitude);
	SetDlgItemText(hDlg, IDC_ARR_ALT, buf);

	SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_RESETCONTENT, 0, 0);
	if (param.n_planets > 0) {
		for (unsigned int i = 0; i < param.n_planets; ++i) {
			char * planet_name = param.planets[i];
			SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_INSERTSTRING, i, (LPARAM)planet_name);
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

	auto plans = g_optimizer->SavedPlans();
	SendDlgItemMessage(hDlg, IDC_PLANS, CB_RESETCONTENT, 0, 0);
	for (unsigned int i = 0; i < plans.size(); ++i) {
		SendDlgItemMessage(hDlg, IDC_PLANS, CB_ADDSTRING, 0, (LPARAM)plans[i].c_str());
	}
}

void MGAFinder::FillCBodyList(HWND hDlg)
{
	int hList = IDC_PLANET_SELECT;

	char cbuf[256];
	SendDlgItemMessage(hDlg, hList, CB_RESETCONTENT, 0, 0);
	auto mercury = oapiGetGbodyByName("Mercury");
	double mercury_size = oapiGetSize(mercury);
	for (DWORD n = 0; n < oapiGetGbodyCount(); ++n) {
		auto body = oapiGetGbodyByIndex(n);
		int type = oapiGetObjectType(body);
		if (type == OBJTP_PLANET) {
			oapiGetObjectName(body, cbuf, 256);
			auto celBody = oapiGetCelbodyInterface(body);
			double objSize = oapiGetSize(body);
			if (objSize < mercury_size) { continue; }
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
		case IDC_RESET_SOL:
			g_optimizer->ResetSolutions();
			break;
		case ID_OPT: {
			UIToParam();
			if (g_optimizer->computing()) {
				g_optimizer->Cancel();
				SetDlgItemText(hDlg, ID_OPT, "Find Optimum");
			} else {
				g_optimizer->RunOptimization(hDlg);
				SetDlgItemText(hDlg, ID_OPT, "Cancel");
			}
			break;
		}
		case ID_PARETO: {
			UIToParam();
			if (!g_optimizer->computing()) {
				g_optimizer->RunPareto(hDlg);
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
		case IDC_SAVE_PLAN:
		{
			UIToParam();
			char buf[1000] = "test";
			GetDlgItemText(hDlg, IDC_PLANS, buf, 999);
			g_optimizer->SavePlan(buf);
			char msg[200];
			sprintf_s(msg, "The plan '%s' was saved to disk", buf);
			int msgBoxID = MessageBox(hDlg, msg, "Plan saved", MB_OK);
		}
		break;
		case IDC_LOAD_PLAN:
		{
			char buf[1000] = "test";
			GetDlgItemText(hDlg, IDC_PLANS, buf, 999);
			g_optimizer->LoadPlan(buf);
			char msg[200];
			sprintf_s(msg, "The plan '%s' was loaded from disk", buf);
			int msgBoxID = MessageBox(hDlg, msg, "Plan loaded", MB_OK);
			ParamToUI(g_optimizer->param());
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
		ParamToUI(g_optimizer->param());

		g_optimizer->SaveCurrentPlan();
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