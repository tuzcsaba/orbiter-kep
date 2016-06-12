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

GravityAssistModule * g_gravityAssist = 0;
MGAFinder * g_mgaFinder = 0;
Optimization * g_optimizer = 0;
MGAModuleMessenger * g_moduleMessenger = 0;

DLLCLBK void InitModule(HINSTANCE hDLL)
{
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
	g_optimizer->SaveStateTo(scn);
}

DLLCLBK void opcLoadState(FILEHANDLE scn) {
	g_optimizer->LoadStateFrom(scn);
}

GravityAssistModule::GravityAssistModule(HINSTANCE _hDLL, Optimization * optimizer) : m_optimizer(optimizer) {
	hDLL = _hDLL;
}

GravityAssistModule::~GravityAssistModule() {
}

bool GravityAssistModule::ShouldDrawHUD() const {
	return m_optimizer->has_solution();
}

void GravityAssistModule::DrawHUD(int mode, const HUDPAINTSPEC *hps, oapi::Sketchpad * skp) {
	if (!m_optimizer->has_solution()) return;

	auto solution = m_optimizer->get_best_solution();
	int W = hps->W;
	int H = hps->H;
	int CX = hps->CX;
	int CY = hps->CY;
	double scale = hps->Scale;

	std::string toDisplay = m_optimizer->get_solution_str_current_stage();

	DrawMultilineString(W - 15, 150, toDisplay, skp);
}

void GravityAssistModule::DrawMultilineString(int right, int top, std::string toDisplay, oapi::Sketchpad * skp) {
	std::vector<std::string> lines;
	boost::algorithm::split(lines, toDisplay, boost::is_any_of("\n"));
	int offsetY = top;
	unsigned int max = 0;
	for (auto line : lines) {
		DWORD width = skp->GetTextWidth(line.c_str());
		if (width > max) max = width;
	}
	for (auto line : lines) {
		if (strlen(line.c_str()) == 0) {
			offsetY += 10;
			continue;
		}
		skp->Text(right - max, offsetY, line.c_str(), line.length());
		offsetY += 20;
	}
	double fuel_cost = m_optimizer->get_best_solution().fuel_cost;
	char str[256];
	sprintf_s(str, "Total DeltaV: %lf", fuel_cost);
	skp->Text(right - max, offsetY, str, strlen(str));
	offsetY += 30;
	if (m_optimizer->computing()) {
		sprintf(str, "Working to find a better solution...");
		skp->Text(right - max, offsetY, str, strlen(str));
		offsetY += 20;
	}
}

void GravityAssistModule::DrawSolution(const HUDPAINTSPEC *hps, oapi::Sketchpad * skp) {

}

const char * GravityAssistModule::GetModuleName() const {
	return "GravityAssistModule";
}
