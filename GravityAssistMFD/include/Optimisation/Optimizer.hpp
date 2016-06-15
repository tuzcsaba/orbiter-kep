#ifndef GRAV_ASSIST_OPTIMIZER_H
#define GRAV_ASSIST_OPTIMIZER_H

#include "orbiterkep/opt/optimise-c.h"
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
	double ** pareto_buffer() { return m_pareto; }
	int *n_pareto() { return &m_n_pareto; }
	void set_computing(bool _computing) { 
		m_computing = _computing; 
	}
	bool cancelled() const { return m_cancel; }
	bool opt_found() const {
		double min = DBL_MAX;
		double max = 0;
		if (n_solutions < 25) return false;
		for (int i = 0; i < n_solutions; ++i) {
			double c = m_solutions[i]->fuel_cost;
			if (c > max) max = c;
			if (c < min) min = c;
		}
		return max - min < 1;
	}
	void update_parameters(Orbiterkep__Parameters * newParam, bool unpacked) {
		int current_hash = param_hash(*m_param);
		int new_hash = param_hash(*newParam);

		if (current_hash != new_hash) {
			SaveCurrentPlan();
		}

		ResetParam();
		m_param = newParam;
		m_param_unpacked = unpacked;
		
		if (current_hash != new_hash) {
			ResetSolutions();
			char cacheFile[40];
			sprintf_s(cacheFile, "cache/%ld", (unsigned int)new_hash);
			LoadPlan(cacheFile);
		}
	}

	std::string get_solution_times() const;

	std::string get_solution_str_current_stage() const;

	void RunPareto(HWND hDlg);
	void RunOptimization(HWND hDlg);

	void Cancel();
	void Update(HWND hDlg);
	void ResetSolutions();
	void ResetParam();

	void LoadScenario(FILEHANDLE scn);
	void SaveScenario(FILEHANDLE scn);
	void SavePlan(char * filename);
	int SaveCurrentPlan();
	void LoadPlan(char * filename);	
	std::vector<std::string> SavedPlans();

	void Signal();

	void AddSolution(Orbiterkep__TransXSolution * newSolution);
private:

	void LoadStateFrom(FILE * scn);
	void SaveStateTo(FILE * scn);

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
	double** m_pareto;
	int m_n_pareto;
	const int max_pareto = 10000;

	const MGAModuleMessenger &m_messenger;
};

#endif