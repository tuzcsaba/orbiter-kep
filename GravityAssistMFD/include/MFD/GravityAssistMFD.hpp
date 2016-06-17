#ifndef GRAVITY_ASSIST_MFD_H
#define GRAVITY_ASSIST_MFD_H

#include <orbitersdk.h>
#include <EnjoLib/IDrawsHUD.hpp>


class GravityAssistMFD : public MFD2, public EnjoLib::IDrawsHUD {
public:
	GravityAssistMFD(DWORD w, DWORD h, VESSEL * vessel, GravityAssistModule * gravAssist);

	/// Destructor
	~GravityAssistMFD();

	virtual bool Update(oapi::Sketchpad *skp);

	static int MsgProc(UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam);

	virtual bool ShouldDrawHUD() const;
	virtual void DrawHUD(int mode, const HUDPAINTSPEC *hps, oapi::Sketchpad * skp);
	virtual const char * GetModuleName() const;
private:
	void DrawSolution(const HUDPAINTSPEC *hps, oapi::Sketchpad * skp);
	void DrawParetoFronts(int offsetY, oapi::Sketchpad * skp);
	int DrawMultilineString(int right, int top, std::string toDisplay, oapi::Sketchpad * skp);

	Optimization *m_optimizer;

	oapi::Font * mfd_font;
	oapi::Font * mfd_graph_font;
};


#endif