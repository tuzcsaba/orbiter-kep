#include "Optimisation/Optimizer.hpp"

#include <Orbitersdk.h>
#include <orbiterkep/opt/optimise-c.h>
#include <orbiterkep/proto/ext-c.h>

#include "ModuleMessaging\MGAModuleMessenger.h"
#include "Dialog/MGAFinder.hpp"

#include <ppltasks.h>
#include <iostream>
#include <string>


struct OptimThreadParam {
	HWND hDlg;
	Optimization * opt;
	MGAPlan * plan;
};


char ** allocate_string_array(int n_str, int max_l) {
	char ** result = (char **)malloc(n_str * sizeof(char *));
	for (int i = 0; i < n_str; ++i) {
		result[i] = (char *)malloc((max_l + 1) * sizeof(char));
	}
	return result;
}

void deallocate_string_array(char * strings[], int n_str) {
	for (int i = 0; i < n_str; ++i) {
		free(strings[i]);
	}
	free(strings);
}

Optimization::~Optimization()
{
	Cancel();

	ResetPlan();
}

Optimization::Optimization(const MGAModuleMessenger &messenger) : m_messenger(messenger)
{
	m_computing = false;

	InitializeDefaultParam();
}

void Optimization::InitializeDefaultParam() {
	m_plan = new MGAPlan(newDefaultParameters());
}

std::string Optimization::get_solution_times() const
{
	char result_buf[10000];
	int len = sprintf_transx_times(result_buf, m_plan->get_best_solution().times);

	return std::string(result_buf, len);
}

std::string Optimization::get_solution_str_current_stage() const
{
	if (!plan()->has_solution()) return "";

	char result_buf[10000];
	int len = 0;


	double currentTime = oapiGetSimMJD();
	auto best = m_plan->get_best_solution();
	if (currentTime <= best.escape->mjd + 1) { // if we are less than 1 day after planned launch, current stage is ESCAPE
		len += sprintf_transx_escape(result_buf + len, m_plan->get_best_solution().escape);
	} else {
		int i = 0; int j = 0;
		int n_dsm = best.n_dsms; int n_flyby = best.n_flybyes;
		bool found = false;
		while (i < n_dsm || j < n_flyby) {
			if (i < n_dsm) {
				if (best.dsms[i]->leg == j) {
					if (currentTime < best.dsms[i]->mjd + 1) {
						len += sprintf_transx_dsm(result_buf + len, best.dsms[i]);
						found = true;
						break;
					}
					++i;
				}
			}
			if (j < n_flyby) {
				if (currentTime < best.flybyes[j]->mjd + 1) {
					len += sprintf_transx_flyby(result_buf + len, best.flybyes[j]);
					found = true;
					break;
				}
				++j;
			}
		}

		if (!found) {
			len += sprintf_transx_arrival(result_buf + len, best.arrival);
		}
	}
	return std::string(result_buf, len);
}

void Optimization::Cancel() {
	m_cancel = true;
}


void RunOptimization_thread(std::shared_ptr<OptimThreadParam> param) {

	char buf[2048];
	int i = 0;
	int len = orbiterkep__parameters__get_packed_size(&param->plan->param());
	orbiterkep__parameters__pack(&param->plan->param(), (uint8_t *)buf);

	char sol_buf[16000];
	int sol_len = 0;

	param->opt->set_computing(true);
	int c = 0;
	while (c < 1 || (!param->opt->plan()->converged() && !param->opt->cancelled())) {
		sol_len = orbiterkep_optimize((const uint8_t *)buf, len, (uint8_t *)sol_buf);

		Orbiterkep__TransXSolution * solution = orbiterkep__trans_xsolution__unpack(NULL, sol_len, (uint8_t *)sol_buf);

		param->opt->plan()->AddSolution(solution);
		param->opt->Signal();
		++c;
	}
	param->opt->Signal();
	param->opt->set_computing(false);
};

void RunOptimization_pareto_thread(std::shared_ptr<OptimThreadParam> param) {

	char buf[2048];
	int i = 0;
	auto plan = param->plan;
	auto par = &plan->param();
	int len = orbiterkep__parameters__get_packed_size(par);
	orbiterkep__parameters__pack(par, (uint8_t *)buf);

	param->opt->set_computing(true);
	int c = 0;
	int n = 10000;
	orbiterkep_optimize_multi((const uint8_t *)buf, len, plan->pareto_buffer(), &n);
	*(plan->get_n_pareto()) = n;
	param->opt->Signal();
	param->opt->set_computing(false);
};

void Optimization::Signal() {
	Update(hDlg);
}

void Optimization::RunOptimization(HWND hDlg)
{
	m_cancel = false;
	
	OptimThreadParam _threadParam;
	auto threadParam = std::make_shared<OptimThreadParam>(_threadParam);
	threadParam->opt = this;
	threadParam->hDlg = hDlg;
	threadParam->plan = m_plan;

	m_optimization_task = concurrency::create_task([threadParam] {
		RunOptimization_thread(threadParam);
	});	
}

void Optimization::RunPareto(HWND hDlg) {
	m_cancel = false;

	OptimThreadParam _threadParam;
	auto threadParam = std::make_shared<OptimThreadParam>(_threadParam);
	threadParam->opt = this;
	threadParam->hDlg = hDlg;
	threadParam->plan = m_plan;

	m_optimization_task = concurrency::create_task([threadParam] {
		RunOptimization_pareto_thread(threadParam);
	});
}

void Optimization::ResetPlan() {
	if (m_plan) {
		delete(m_plan);
		m_plan = 0;
	}
}

std::vector<std::string> Optimization::SavedPlans() {
	WIN32_FIND_DATA fdFile;
	HANDLE hFind = NULL;


	std::vector<std::string> result;
	char sPath[MAX_PATH];
	sprintf_s(sPath, "%s\\*.mga", "Config/MFD/GravityAssistMFD/MGAPlans");

	if ((hFind = FindFirstFile(sPath, &fdFile)) == INVALID_HANDLE_VALUE) {
		return result;
	}
	do {
		if (strcmp(fdFile.cFileName, ".") == 0
			|| strcmp(fdFile.cFileName, "..") == 0) continue;

		std::string s(fdFile.cFileName, strlen(fdFile.cFileName) - 4);
		result.push_back(s);
	} while (FindNextFile(hFind, &fdFile));

	return result;
}


void Optimization::Update(HWND _hDlg) {
	if (m_plan->has_solution()) {
		auto best = m_plan->get_best_solution();
		m_messenger.PutSolution(best);
	}

	if (_hDlg != NULL) {
		hDlg = _hDlg;
	}
	if (hDlg != 0) {
		PostMessage(hDlg, WM_OPTIMIZATION_READY, 0, 0);
	}
}
