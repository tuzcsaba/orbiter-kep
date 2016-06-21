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

	auto plan = g_optimizer->plan();
	if (plan) {
		MGAFinder::ParamToUI(*plan);
	}

	g_optimizer->Update(_hDlg);
}

void MGAFinder::OpenDialog()
{
	hDlg = oapiOpenDialogEx(hInst, IDD_MGA_FINDER, MGAFinderProc, 
		0, this);
}
	
void MGAFinder::CloseDialog()
{
	if (hDlg) {
		oapiCloseDialog(hDlg);
		hDlg = NULL;
	}
}

const int N_KNOWN_PLANETS = 10;
const char * KNOWN_PLANETS[] = {
	"mercury",
	"venus",
	"earth",
	"mars",
	"jupiter",
	"saturn",
	"uranus",
	"neptune",
	"pluto",
	"DSM"
};

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

	int nPlanets = SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_GETCOUNT, 0, 0) + 2;
	newParam->planets = (char **)malloc(sizeof(char *) * nPlanets);
	GetDlgItemText(hDlg, IDC_PLANET_DEP, buf, 255);
	newParam->planets[0] = (char *)malloc(sizeof(char) * (strlen(buf) + 1));
	strcpy(newParam->planets[0], buf);

	int omit_dsm_cnt = nPlanets - 1;
	protobuf_c_boolean * dsms = (protobuf_c_boolean *)malloc(sizeof(protobuf_c_boolean *) * omit_dsm_cnt);
	int j = 0;
	int k = 1;
	bool expect_dsm = true;
	bool DSM_spec = false;
	int cnt = SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_GETCOUNT, 0, 0);
	for (int i = 1; i < cnt + 1; ++i) {
		SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_GETTEXT, i - 1, (LPARAM)buf);
		if (!strcmp(buf, "DSM")) {
			expect_dsm = false;
			DSM_spec = true;
			dsms[j++] = 1;
			nPlanets -= 1;
		} else {
			newParam->planets[k] = (char *)malloc(sizeof(char) * (strlen(buf) + 1));
			strcpy(newParam->planets[k], buf);
			k += 1;
			if (!expect_dsm) {
				expect_dsm = true;
			} else {
				dsms[j++] = 0;
			}
		}
	}
	GetDlgItemText(hDlg, IDC_PLANET_ARR, buf, 255);
	newParam->planets[k] = (char *)malloc(sizeof(char) * (strlen(buf) + 1));
	strcpy(newParam->planets[k], buf);

	newParam->n_planets = nPlanets;
	if (DSM_spec) {
		newParam->n_allow_dsm = j;
		newParam->allow_dsm = dsms;
	} else {
		newParam->n_allow_dsm = 0;
		newParam->allow_dsm = NULL;
		free(dsms);
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

	MGAPlan * plan = new MGAPlan(newParam);

	g_optimizer->update_parameters(plan, false);
}

void MGAFinder::ParamToUI(const MGAPlan &plan)
{
	auto param = plan.param();
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
		char * planet_name = param.planets[0];
		SetDlgItemText(hDlg, IDC_PLANET_DEP, planet_name);
		int j = 0;
		unsigned int i = 0;
		int c = 0;
		for (i = 1; i < param.n_planets - 1; ++i) {
			if (param.n_allow_dsm > 0) {
				if (j < param.n_allow_dsm) {
					if (param.allow_dsm[j]) {
						planet_name = (char *)malloc(sizeof(char *) * 4);
						strcpy(planet_name, "DSM");
						SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_INSERTSTRING, i - 1 + c, (LPARAM)planet_name);
						free(planet_name);
						++c;
					}
					++j;
				}
			}
			planet_name = param.planets[i];
			SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_INSERTSTRING, i - 1 + c, (LPARAM)planet_name);
		}
		if (param.n_allow_dsm > 0) {
			if (j < param.n_allow_dsm) {
				if (param.allow_dsm[j]) {
					planet_name = (char *)malloc(sizeof(char *) * 4);
					strcpy(planet_name, "DSM");
					SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_INSERTSTRING, i - 1 + c, (LPARAM)planet_name);
					free(planet_name);
					++c;
				}
				++j;
			}
		}

		planet_name = param.planets[param.n_planets - 1];
		SetDlgItemText(hDlg, IDC_PLANET_ARR, planet_name);
	}

	SendDlgItemMessage(hDlg, IDC_PROBLEM, CB_SELECTSTRING, -1, (LPARAM)param.problem);

	if (plan.has_solution()) {
		SetDlgItemText(hDlg, IDC_SOLUTION, plan.get_solution_str(plan.get_best_solution()).c_str());
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
	int hLists[] = { IDC_PLANET_SELECT, IDC_PLANET_DEP, IDC_PLANET_ARR};

	for (int hList : hLists) {
		SendDlgItemMessage(hDlg, hList, CB_RESETCONTENT, 0, 0);
		DWORD max_n = (hList == IDC_PLANET_SELECT) ? N_KNOWN_PLANETS : (N_KNOWN_PLANETS - 1);
		for (DWORD n = 0; n < max_n; ++n) {
			SendDlgItemMessage(hDlg, hList, CB_ADDSTRING, 0, (LPARAM)KNOWN_PLANETS[n]);
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


void MGAFinder::SolutionToUI(const MGAPlan &plan) {
	SetDlgItemText(hDlg, IDC_SOLUTION, plan.get_solution_str(plan.get_best_solution()).c_str());
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

void MGAFinder::PlanetList_ExchangeItemsAtIndex(int idx1, int idx2) {
	if (idx1 == idx2) return;

	char buf1[256];
	char buf2[256];
	int x = min(idx1, idx2); int y = max(idx1, idx2);
	SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_GETTEXT, x, (LPARAM)buf1);
	SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_GETTEXT, y, (LPARAM)buf2);	
	PlanetList_RemoveItemAtIndex(x);
	PlanetList_RemoveItemAtIndex(y - 1);
	PlanetList_InsertItemAtIndex(buf2, x);
	PlanetList_InsertItemAtIndex(buf1, y);

	SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_SETCURSEL, idx2, 0);
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
			g_optimizer->plan()->ResetSolutions();
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
		case IDC_PLUS: {
			int idx = SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_GETCURSEL, 0, 0);
			int cnt = SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_GETCOUNT, 0, 0);
			if (idx >= 0 && idx < cnt - 1) {
				PlanetList_ExchangeItemsAtIndex(idx, idx + 1);
			}
		}
			break;
		case IDC_MINUS: {
			int idx = SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_GETCURSEL, 0, 0);
			int cnt = SendDlgItemMessage(hDlg, IDC_LST_PLANETS, LB_GETCOUNT, 0, 0);
			if (idx >= 1) {
				PlanetList_ExchangeItemsAtIndex(idx, idx - 1);
			}
		}
			break;
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
			g_optimizer->disk_store().SavePlan(buf, g_optimizer->plan());
			char msg[200];
			sprintf_s(msg, "The plan '%s' was saved to disk", buf);
			int msgBoxID = MessageBox(hDlg, msg, "Plan saved", MB_OK);
		}
		break;
		case IDC_LOAD_PLAN:
		{
			char buf[1000] = "test";
			GetDlgItemText(hDlg, IDC_PLANS, buf, 999);
			auto plan = g_optimizer->disk_store().LoadPlan(buf);
			g_optimizer->update_parameters(plan, false);
			char msg[200];
			sprintf_s(msg, "The plan '%s' was loaded from disk", buf);
			int msgBoxID = MessageBox(hDlg, msg, "Plan loaded", MB_OK);
			if (plan) {
				ParamToUI(*plan);
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
		auto plan = g_optimizer->plan();
		if (plan) {
			SolutionToUI(*plan);
			ParamToUI(*plan);

			g_optimizer->disk_store().SavePlan(plan);
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