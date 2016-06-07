#define STRICT 1
#define ORBITER_MODULE

#include "GravityAssistMFD.h"

#include "Orbitersdk.h"

#include "DlgCtrl.h"
#include "MGAFinder.h"
#include <windows.h>
#include <CommCtrl.h>

MGAFinder *g_mgaFinder = 0;

DLLCLBK void InitModule(HINSTANCE hDLL)
{
	INITCOMMONCONTROLSEX cc = { sizeof(INITCOMMONCONTROLSEX), ICC_TREEVIEW_CLASSES };
	InitCommonControlsEx(&cc);

	g_mgaFinder = new MGAFinder(hDLL);

	oapiRegisterCustomControls(hDLL);
}

DLLCLBK void ExitModule(HINSTANCE hDLL)
{
	delete g_mgaFinder;
	g_mgaFinder = 0;

	oapiUnregisterCustomControls(hDLL);
}