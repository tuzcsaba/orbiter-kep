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

	MGAFinder(HINSTANCE hDLL);
	~MGAFinder();
	void OpenDialog();
	void CloseDialog();
	void InitDialog(HWND hDlg);

	int MsgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	HWND DlgHandle() const { return hDlg; }
	HINSTANCE InstHandle() const { return hInst; }

	void FillCBodyList(HWND hDlg);
	void FillProblemList(HWND hDlg);
protected:

	/** Planet list */
	void PlanetList_AddItem(char * item);
	void PlanetList_RemoveItem(char * item);
	void PlanetList_RemoveItemAtIndex(int idx);
	void PlanetList_InsertItemAtIndex(char * item, int idx);
	void PlanetList_ExchangeItemsAtIndex(int idx1, int idx2);

private:
	void ParamToUI(const MGAPlan &plan);
	void SolutionToUI(const MGAPlan &plan);
	void UIToParam();

	Optimization  * m_optimizer;

	DWORD dwCmd;
	HWND hDlg;
	HINSTANCE hInst;	
};

#endif