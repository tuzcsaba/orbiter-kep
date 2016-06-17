#define STRICT
#define ORBITER_MODULE

#include "Dialog/MGAFinder.hpp"

#include "GravityAssistModule.h"

#include "MFD/GravityAssistMFD.hpp"
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
	mfd_graph_font = oapiCreateFont(h / 40, true, "Fixed", FONT_NORMAL, 0);
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
	oapiReleaseFont(mfd_graph_font);
	mfd_font = 0;
	mfd_graph_font = 0;
}


bool GravityAssistMFD::ShouldDrawHUD() const {
	return m_optimizer->has_solution();
}

bool GravityAssistMFD::Update(oapi::Sketchpad * skp) {

	Title(skp, "GravityAssist MFD");

	if (!m_optimizer->has_solution() && *(m_optimizer->n_pareto()) == 0) return true;

	int offsetY = 0;
	int h = 0;
	if (m_optimizer->has_solution()) {
		skp->SetFont(mfd_font);

		h = LOWORD(skp->GetCharSize());
		std::string toDisplay = m_optimizer->get_solution_str_current_stage();
		offsetY = DrawMultilineString(15, 40, toDisplay, skp);
		offsetY -= h;
	}

	if (*(m_optimizer->n_pareto()) > 0) {
		DrawParetoFronts(offsetY, skp);
	}
	return true;
}

void GravityAssistMFD::DrawParetoFronts(int offsetY, oapi::Sketchpad * skp)
{
	int bottom_margin = 25;
	skp->SetFont(mfd_graph_font);
	int h = LOWORD(skp->GetCharSize());
	skp->Line(15, offsetY + 10, 15, H - bottom_margin);
	skp->Line(10, offsetY + 10 + 5, 15, offsetY + 10);
	skp->Line(20, offsetY + 10 + 5, 15, offsetY + 10);

	skp->Line(15, H - bottom_margin, W - 15, H - bottom_margin);
	skp->Line(W - 20, H - bottom_margin - 5, W - 15, H - bottom_margin);
	skp->Line(W - 20, H - bottom_margin + 5, W - 15, H - bottom_margin);

	int n = *(m_optimizer->n_pareto());
	double ** p = m_optimizer->pareto_buffer();
	double minT = DBL_MAX;
	double maxT = 0;
	double minDV = DBL_MAX;
	double maxDV = 0;
	for (int i = 0; i < n; ++i) {
		if (p[i][0] < minDV) minDV = p[i][0];
		if (p[i][0] > maxDV) maxDV = p[i][0];
		if (p[i][1] < minT) minT = p[i][1];
		if (p[i][1] > maxT) maxT = p[i][1];
	}
	double scaleT = (H - bottom_margin - offsetY - 20) / (maxT - minT);
	double scaleDV = (W - 35) / (maxDV - minDV);
	for (int i = 0; i < n; ++i) {
		double x = (p[i][0] - minDV) * scaleDV;
		double y = (p[i][1] - minT) * scaleT;
		int screenX = (int)floor(15 + x);
		int screenY = (int)floor(H - bottom_margin - y);
		skp->Pixel(screenX, screenY, 0xFFFFFF);
	}

	/** T markers */
	double ratio = 1;
	char numbuf[100];
	int offs = 0;
	for (int i = 0; i < 4; ++i) {

		offs = (int)floor(offsetY + 20 + (H - bottom_margin - offsetY - 20) * (1 - ratio));
		skp->Line(10, offs, 20, offs);
		sprintf(numbuf, "%0.1lf", minT + (maxT - minT) * (ratio));
		skp->Text(25, offs - h / 2, numbuf, strlen(numbuf));
		ratio /= 2;
	}
	/** DV markers */
	ratio = 1;
	for (int i = 0; i < 4; ++i) {

		offs = (int)floor(15 + (W - 35) * ratio);
		skp->Line(offs, H - bottom_margin - 5, offs, H - bottom_margin + 5);
		sprintf(numbuf, "%0.1lf", minDV + (maxDV - minDV) * (ratio));
		int w = skp->GetTextWidth(numbuf, 0);
		skp->Text(offs - w / 2, H - bottom_margin + 10, numbuf, strlen(numbuf));
		ratio /= 2;
	}
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

	int left = W - 700;
	int offsetY = DrawMultilineString(left, 150, toDisplay, skp);

	int h = LOWORD(skp->GetCharSize());
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

int GravityAssistMFD::DrawMultilineString(int left, int top, std::string toDisplay, oapi::Sketchpad * skp) {
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
			offsetY += (int)floor( h / 2.0);
			continue;
		}
		skp->Text(left, offsetY, line.c_str(), line.length());
		offsetY += h;
	}
	return offsetY;
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
