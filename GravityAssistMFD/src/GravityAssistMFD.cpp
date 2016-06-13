#define STRICT
#define ORBITER_MODULE

#include "Dialog/MGAFinder.hpp"

#include "GravityAssistMFD.hpp"
#include "ModuleMessaging\MGAModuleMessenger.h"

#include <orbitersdk.h>

#include "DlgCtrl.h"

#include <boost/algorithm/string.hpp>
#include <stdlib.h>
#include <vector>
#include <windows.h>
#include <CommCtrl.h>

int g_MFDmode;

GravityAssistModule * g_gravityAssist = 0;
MGAFinder * g_mgaFinder = 0;
Optimization * g_optimizer = 0;
MGAModuleMessenger * g_moduleMessenger = 0;

GravityAssistMFD::GravityAssistMFD(DWORD w, DWORD h, VESSEL *vessel, GravityAssistModule * gravAssist) : MFD2(w, h, vessel), m_optimizer(&(gravAssist->optimizer()))
{
	mfd_font = oapiCreateFont(h / 20, true, "Fixed", FONT_NORMAL, 0);
}

int GravityAssistMFD::MsgProc(UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case OAPI_MSG_MFD_OPENED:
		return (int)new GravityAssistMFD(LOWORD(wparam), HIWORD(wparam), (VESSEL *)lparam, g_gravityAssist);
	}
	return 0;
}

// Destructor
GravityAssistMFD::~GravityAssistMFD()
{
	// Add MFD cleanup code here
	oapiReleaseFont(mfd_font);
	mfd_font = 0;
}


bool GravityAssistMFD::ShouldDrawHUD() const {
	return m_optimizer->has_solution();
}

bool GravityAssistMFD::Update(oapi::Sketchpad * skp) {
	Title(skp, "GravityAssist MFD");
	skp->SetFont(mfd_font);

	std::string toDisplay = m_optimizer->get_solution_str_current_stage();
	DrawMultilineString(15, 40, toDisplay, skp);

	return true;
}

void GravityAssistMFD::DrawHUD(int mode, const HUDPAINTSPEC *hps, oapi::Sketchpad * skp) {
	if (!m_optimizer->has_solution()) return;

	auto solution = m_optimizer->get_best_solution();
	int W = hps->W;
	int H = hps->H;
	int CX = hps->CX;
	int CY = hps->CY;
	double scale = hps->Scale;

	std::string toDisplay = m_optimizer->get_solution_times();

	DrawMultilineString(W - 700, 150, toDisplay, skp);
}

void GravityAssistMFD::DrawMultilineString(int left, int top, std::string toDisplay, oapi::Sketchpad * skp) {
	std::vector<std::string> lines;
	boost::algorithm::split(lines, toDisplay, boost::is_any_of("\n"));
	int h = LOWORD(skp->GetCharSize());

	int offsetY = top;
	unsigned int max = 0;
	for (auto line : lines) {
		DWORD width = skp->GetTextWidth(line.c_str());
		if (width > max) max = width;
	}
	for (auto line : lines) {
		if (strlen(line.c_str()) == 0) {
			offsetY += (int)floor(3 * h / 4);
			continue;
		}
		skp->Text(left, offsetY, line.c_str(), line.length());
		offsetY += h;
	}
	double fuel_cost = m_optimizer->get_best_solution().fuel_cost;
	char str[256];
	sprintf_s(str, "Total DeltaV: %lf", fuel_cost);
	skp->Text(left, offsetY, str, strlen(str));
	offsetY += (int)floor(7 * h / 4);
	if (m_optimizer->computing()) {
		sprintf(str, "Working to find a better solution...");
		skp->Text(left, offsetY, str, strlen(str));
		offsetY += h;
	}
}

void GravityAssistMFD::DrawSolution(const HUDPAINTSPEC *hps, oapi::Sketchpad * skp) {

}

const char * GravityAssistMFD::GetModuleName() const {
	return "GravityAssistModule";
}

DLLCLBK void InitModule(HINSTANCE hDLL)
{
	static char * name = "GravityAssistMFD";
	MFDMODESPEC spec;
	spec.name = name;
	spec.key = OAPI_KEY_T;
	spec.msgproc = GravityAssistMFD::MsgProc;

	g_MFDmode = oapiRegisterMFDMode(spec);

	INITCOMMONCONTROLSEX cc = { sizeof(INITCOMMONCONTROLSEX), ICC_TREEVIEW_CLASSES };
	InitCommonControlsEx(&cc);

	g_moduleMessenger = new MGAModuleMessenger();
	g_optimizer = new Optimization(*g_moduleMessenger);
	g_mgaFinder = new MGAFinder(hDLL);

	g_gravityAssist = new GravityAssistModule(hDLL, g_optimizer);

	oapiRegisterCustomControls(hDLL);
}


DLLCLBK void ExitModule(HINSTANCE hDLL)
{
	oapiUnregisterCustomControls(hDLL);
	oapiUnregisterMFDMode(g_MFDmode);

	g_optimizer->Cancel();

	delete g_gravityAssist;
	g_gravityAssist = 0;

	delete g_mgaFinder;
	g_mgaFinder = 0;

	delete g_optimizer;
	g_optimizer = 0;

	delete g_moduleMessenger;
	g_moduleMessenger = 0;

}

DLLCLBK void opcSaveState(FILEHANDLE scn) {
	g_optimizer->SaveScenario(scn);
}

DLLCLBK void opcLoadState(FILEHANDLE scn) {
	g_optimizer->LoadScenario(scn);
}

GravityAssistModule::GravityAssistModule(HINSTANCE _hDLL, Optimization * optimizer) : m_optimizer(optimizer) {
	hDLL = _hDLL;
}

GravityAssistModule::~GravityAssistModule() {
}
