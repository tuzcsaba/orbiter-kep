#ifndef __MGAFinder_H
#define __MGAFinder_H

#include "EnjoLib/IDrawsHUD.hpp"

#include "Optimisation/Optimizer.hpp"

#include <string>

#include <Windows.h>
#include <CommCtrl.h>

#include "orbiterkep/proto/orbiterkep-proto_Export.h"
#include "orbiterkep/proto/solution.pb-c.h"
#include "orbiterkep/proto/parameters.pb-c.h"

const int WM_OPTIMIZATION_READY = WM_USER + 314;



class MGAFinder {
public:

	MGAFinder(HINSTANCE hDLL, Optimization &optimizer);
	~MGAFinder();
	void OpenDialog();
	void CloseDialog();
	void InitDialog(HWND hDlg);

	const Optimization &get_optimizer() { return m_optimizer; }

	int MsgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	HWND DlgHandle() const { return hDlg; }
	HINSTANCE InstHandle() const { return hInst; }

	void FillCBodyList(HWND hDlg);
	void FillProblemList(HWND hDlg);
protected:

	/** Planet list */
	void MGAFinder::PlanetList_AddItem(char * item);
	void MGAFinder::PlanetList_RemoveItem(char * item);
	void MGAFinder::PlanetList_RemoveItemAtIndex(int idx);
	void MGAFinder::PlanetList_InsertItemAtIndex(char * item, int idx);

private:
	void Optimize();
	void ParamToUI(const Orbiterkep__Parameters &param);
	void SolutionToUI(const Orbiterkep__TransXSolution &solution);
	void UIToParam(Orbiterkep__Parameters &param);

	Optimization &m_optimizer;

	DWORD dwCmd;
	HWND hDlg;
	HINSTANCE hInst;	
};

#endif