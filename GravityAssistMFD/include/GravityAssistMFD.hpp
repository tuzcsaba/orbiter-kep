#ifndef GRAVITY_ASSIST_MFD_H
#define GRAVITY_ASSIST_MFD_H

#include <orbitersdk.h>
#include <EnjoLib/IDrawsHUD.hpp>

class Optimization;

class GravityAssistModule : public EnjoLib::IDrawsHUD {

public:

	GravityAssistModule(HINSTANCE hDLL, Optimization * optimizer);

	/// Destructor
	~GravityAssistModule();

	bool ShouldDrawHUD() const;
	void DrawHUD(int mode, const HUDPAINTSPEC *hps, oapi::Sketchpad * skp);
	const char * GetModuleName() const;

	void DrawSolution(const HUDPAINTSPEC *hps, oapi::Sketchpad * skp);
	void DrawMultilineString(int right, int top, std::string toDisplay, oapi::Sketchpad * skp);

protected:
	MGAFinder *m_mgaFinder;
	Optimization *m_optimizer;
	HINSTANCE hDLL;

};

#endif