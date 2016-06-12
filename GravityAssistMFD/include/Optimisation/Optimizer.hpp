#ifndef GRAV_ASSIST_OPTIMIZER_H
#define GRAV_ASSIST_OPTIMIZER_H

#include "orbiterkep/proto/solution.pb-c.h"
#include "orbiterkep/proto/parameters.pb-c.h"

#include <Orbitersdk.h>

#include <ppltasks.h>
#include <string>
#include <vector>
#include <Windows.h>

class MGAModuleMessenger;

struct OptimThreadParam;

class Optimization
{
public:
	Optimization(const MGAModuleMessenger &m_messenger);
	~Optimization();
	void InitializeDefaultParam();

	const Orbiterkep__TransXSolution &get_best_solution() const { return *m_best_solution; };
	std::string get_solution_str() const { return m_solution_str; };
	Orbiterkep__Parameters & param() const { return *m_param; };
	bool has_solution() const { return n_solutions > 0; }
	int get_n_solutions() const { return n_solutions;  }
	bool computing() const { return m_computing; }
	void set_computing(bool _computing) { 
		m_computing = _computing; 
	}
	bool cancelled() const { return m_cancel; }
	void update_parameters(Orbiterkep__Parameters * newParam, bool unpacked) {
		ResetParam();
		m_param = newParam;
		m_param_unpacked = unpacked;
	}

	std::string get_solution_str_current_stage() const;

	void RunOptimization(HWND hDlg);

	void Cancel();
	void Update(HWND hDlg);
	void ResetSolutions();
	void ResetParam();

	void LoadStateFrom(FILEHANDLE scn);
	void SaveStateTo(FILEHANDLE scn);

	void SavePlan(char * filename);
	void LoadPlan(char * filename);
	std::vector<std::string> SavedPlans();

	void Signal();

	void AddSolution(Orbiterkep__TransXSolution * newSolution);
private:
	void free_manual_param();

	std::string m_solution_str;
	bool m_computing;
	bool m_cancel;

	concurrency::task<void> m_optimization_task;
	HWND hDlg;
	
	Orbiterkep__Parameters * m_param;
	bool m_param_unpacked;
	int n_solutions;
	Orbiterkep__TransXSolution ** m_solutions;
	Orbiterkep__TransXSolution * m_best_solution;

	const MGAModuleMessenger &m_messenger;
};

#endif