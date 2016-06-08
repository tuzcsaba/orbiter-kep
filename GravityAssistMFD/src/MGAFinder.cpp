#include "MGAFinder.h"

#include "resource.h"

#include "orbiterkep/opt/optimise-c.h"

#include <Orbitersdk.h>

#include "DlgCtrl.h"
#include <stdio.h>
#include <io.h>

#include "orbiterkep/proto/orbiterkep-proto_Export.h"
#include "orbiterkep/proto/solution.pb-c.h"
#include "orbiterkep/proto/parameters.pb-c.h"

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

	/*
	orbiterkep::Parameters param;
	std::string param_str;

	param.SerializeToString(&param_str);
	int param_l = param_str.length();
	const char * buf = param_str.c_str();

	char sol_buf[2000];

	int sol_len = orbiterkep_optimize(buf, param_l, sol_buf);

	std::string sol_str(sol_buf, sol_len);
	orbiterkep::TransXSolution solution;
	solution.ParseFromString(sol_str);

	char * hello = "Hello";
	*/
	Orbiterkep__Parameters param = ORBITERKEP__PARAMETERS__INIT;
	char * planets[] = { "earth", "venus", "mercury" };
	param.n_planets = 3;
	param.planets = planets;
	char * single_algos[] = { "jde" };
	param.n_single_objective_algos = 1;
	param.single_objective_algos = single_algos;
	char * multi_algos[] = { "nsga2" };
	param.n_multi_objective_algos = 1;
	param.multi_objective_algos = multi_algos;

	param.problem = "MGA";

	param.has_n_trials = 1;
	param.n_trials = 1;

	param.has_n_gen = 1;
	param.n_gen = 100000;

	Orbiterkep__ParamBounds t0 = ORBITERKEP__PARAM_BOUNDS__INIT;
	t0.has_lb = 1; t0.has_ub = 1;
	t0.lb = 51000; t0.ub = 56000;
	param.t0 = &t0;

	Orbiterkep__ParamBounds tof = ORBITERKEP__PARAM_BOUNDS__INIT;
	tof.has_lb = 1; tof.has_ub = 1;
	tof.lb = 0.1; tof.ub = 5.0;
	param.tof = &tof;

	Orbiterkep__ParamBounds vinf = ORBITERKEP__PARAM_BOUNDS__INIT;
	vinf.has_lb = 1; vinf.has_ub = 1;
	vinf.lb = 0.1; vinf.ub = 12.0;
	param.vinf = &vinf;

	param.has_max_deltav = 1;
	param.max_deltav = 24.0;

	param.has_circularize = 1;
	param.circularize = 1;

	param.has_add_arr_vinf = 1;
	param.add_arr_vinf = 1;

	param.has_add_dep_vinf = 1;
	param.add_dep_vinf = 1;

	param.has_dep_altitude = 1;
	param.dep_altitude = 300;

	param.has_arr_altitude = 1;
	param.arr_altitude = 300;

	param.has_multi_objective = 1;
	param.multi_objective = 0;

	param.has_use_db = 1;
	param.use_db = 0;

	param.has_use_spice = 1;
	param.use_spice = 0;

	int len = orbiterkep__parameters__get_packed_size(&param);
	void * buf = malloc(len);
	orbiterkep__parameters__pack(&param, (uint8_t *)buf);

	void * sol_buf = malloc(2000);
	int sol_len = orbiterkep_optimize((const uint8_t *)buf, len, (uint8_t *)sol_buf);
	Orbiterkep__TransXSolution * solution = orbiterkep__trans_xsolution__unpack(NULL, sol_len, (uint8_t *)sol_buf);

	orbiterkep__trans_xsolution__free_unpacked(solution, NULL);

	printf("Hello");
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