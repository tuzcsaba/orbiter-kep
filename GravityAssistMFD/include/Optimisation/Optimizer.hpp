#ifndef GRAV_ASSIST_OPTIMIZER_H
#define GRAV_ASSIST_OPTIMIZER_H

#include "orbiterkep/proto/solution.pb-c.h"
#include "orbiterkep/proto/parameters.pb-c.h"

#include <Orbitersdk.h>

#include <string>
#include <vector>
#include <Windows.h>

class MGAModuleMessenger;

class Optimization
{
public:
	Optimization(const MGAModuleMessenger &m_messenger);
	~Optimization();

	const Orbiterkep__TransXSolution &get_best_solution() const { return *m_best_solution; };
	std::string get_solution_str() const { return m_solution_str; };
	Orbiterkep__Parameters & param() const { return *m_param; };
	bool has_solution() const { return n_solutions > 0; }
	bool computing() const { return m_computing; }

	std::string get_solution_str_current_stage() const;

	void RunOptimization(HWND hDlg);
	void Cancel();
	void Update(HWND hDlg);
	void ResetSolutions();

	void LoadStateFrom(FILEHANDLE scn);
	void SaveStateTo(FILEHANDLE scn);
private:
	void AddSolution(Orbiterkep__TransXSolution * newSolution);

	std::string m_solution_str;
	bool m_computing;
	bool m_cancel;
	
	Orbiterkep__Parameters * m_param;
	int n_solutions;
	Orbiterkep__TransXSolution ** m_solutions;
	Orbiterkep__TransXSolution * m_best_solution;

	const MGAModuleMessenger &m_messenger;
};

#endif