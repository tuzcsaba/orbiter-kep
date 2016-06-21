#ifndef GRAV_ASSIST_OPTIMIZER_H
#define GRAV_ASSIST_OPTIMIZER_H

#include "orbiterkep/opt/optimise-c.h"
#include "orbiterkep/proto/solution.pb-c.h"
#include "orbiterkep/proto/parameters.pb-c.h"

#include "Optimisation/MGAPlan.hpp"
#include "Optimisation/SolutionDiskStore.hpp"

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

	MGAPlan * const plan() const { return m_plan; }
	SolutionDiskStore & disk_store() { return m_disk_store; };

	bool computing() const { return m_computing; }
	void set_computing(bool _computing) { 
		m_computing = _computing; 
	}
	bool cancelled() const { return m_cancel; }

	void update_parameters(MGAPlan * newPlan, bool unpacked) {
		int current_hash = param_hash(m_plan->param());
		int new_hash = param_hash(newPlan->param());

		m_disk_store.SavePlan(m_plan);
		
		if (current_hash != new_hash) {
			ResetPlan();
			m_plan = newPlan;
		} else {
			m_plan->UpdateParam(&newPlan->param());
		}
	}

	std::string get_solution_times() const;

	std::string get_solution_str_current_stage() const;

	void RunPareto(HWND hDlg);
	void RunOptimization(HWND hDlg);

	void Cancel();
	void Update(HWND hDlg);
	void ResetPlan();

	std::vector<std::string> SavedPlans();

	void Signal();

private:

	void free_manual_param();

	MGAPlan * m_plan;

	SolutionDiskStore m_disk_store;

	bool m_computing;
	bool m_cancel;

	concurrency::task<void> m_optimization_task;

	const MGAModuleMessenger &m_messenger;

	HWND hDlg;
};

#endif