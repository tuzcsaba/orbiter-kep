#include "Optimisation/MGAPlan.hpp"

#include <orbiterkep/opt/optimise-c.h>
#include <orbiterkep/proto/ext-c.h>

char ** string_array(char * source[], int n) {
	char ** res = (char **)malloc(sizeof(char *) * n);
	for (int i = 0; i < n; ++i) {
		res[i] = (char *)malloc(sizeof(char) * (strlen(source[i]) + 1));
		strcpy_s(res[i], strlen(source[i]) + 1, source[i]);
	}
	return res;
}

Orbiterkep__Parameters * newDefaultParameters() {
	Orbiterkep__Parameters * m_param = (Orbiterkep__Parameters *)malloc(sizeof(Orbiterkep__Parameters));
	orbiterkep__parameters__init(m_param);

	m_param->n_planets = 3;
	char * planets[] = { "earth", "venus", "mercury" };
	m_param->planets = string_array(planets, 3);

	char * single_obj_algos[] = { "jde" };
	m_param->n_single_objective_algos = 1;
	m_param->single_objective_algos = string_array(single_obj_algos, 1);

	char * multi_obj_algos[] = { "nsga2" };
	m_param->n_multi_objective_algos = 1;
	m_param->multi_objective_algos = string_array(single_obj_algos, 1);

	m_param->problem = (char *)malloc(sizeof(char) * 4);
	strcpy_s(m_param->problem, 4, "MGA");

	Orbiterkep__ParamBounds * t0 = (Orbiterkep__ParamBounds *)malloc(sizeof(Orbiterkep__ParamBounds));
	orbiterkep__param_bounds__init(t0);
	t0->has_lb = 1; t0->has_ub = 1;
	t0->lb = 51000; t0->ub = 56000;
	m_param->t0 = t0;

	Orbiterkep__ParamBounds * tof = (Orbiterkep__ParamBounds *)malloc(sizeof(Orbiterkep__ParamBounds));
	orbiterkep__param_bounds__init(tof);
	tof->has_lb = 1; tof->has_ub = 1;
	tof->lb = 0.1; tof->ub = 5.0;
	m_param->tof = tof;

	Orbiterkep__ParamBounds * vinf = (Orbiterkep__ParamBounds *)malloc(sizeof(Orbiterkep__ParamBounds));
	orbiterkep__param_bounds__init(vinf);
	vinf->has_lb = 1; vinf->has_ub = 1;
	vinf->lb = 0.1; vinf->ub = 12.0;
	m_param->vinf = vinf;

	Orbiterkep__ParamPaGMO * pagmo = (Orbiterkep__ParamPaGMO *)malloc(sizeof(Orbiterkep__ParamPaGMO));
	orbiterkep__param_pa_gmo__init(pagmo);
	pagmo->has_n_isl = 1; pagmo->n_isl = 8;
	pagmo->has_n_gen = 1; pagmo->n_gen = 100000;
	pagmo->has_population = 1; pagmo->population = 60;
	pagmo->has_mf = 1; pagmo->mf = 150;
	pagmo->has_mr = 1; pagmo->mr = 0.15;
	m_param->pagmo = pagmo;

	m_param->has_circularize = 1;
	m_param->circularize = 1;

	m_param->has_add_arr_vinf = 1;
	m_param->add_arr_vinf = 1;

	m_param->has_add_dep_vinf = 1;
	m_param->add_dep_vinf = 1;

	m_param->has_n_trials = 1;
	m_param->n_trials = 1;

	m_param->has_max_deltav = 1;
	m_param->max_deltav = 24000;

	m_param->has_dep_altitude = 1;
	m_param->dep_altitude = 300;

	m_param->has_arr_altitude = 1;
	m_param->arr_altitude = 300;

	m_param->has_multi_objective = 1;
	m_param->multi_objective = 0;

	m_param->has_use_db = 1;
	m_param->use_db = 0;

	m_param->has_use_spice = 1;
	m_param->use_spice = 0;

	m_param->n_allow_dsm = 0;
	m_param->allow_dsm = NULL;

	int hash = param_hash(*m_param);

	char cacheFile[40];
	sprintf_s(cacheFile, "cache/%ld", (unsigned int)hash);

	return m_param;
}

MGAPlan::MGAPlan(Orbiterkep__Parameters * params) : m_param(params), n_solutions(0), m_n_pareto(0), m_best_solution(0) {

	m_solutions = (Orbiterkep__TransXSolution **)malloc(25 * sizeof(Orbiterkep__TransXSolution *));
	m_pareto = (double **)malloc(sizeof(double*) * max_pareto);
	for (int i = 0; i < max_pareto; ++i) m_pareto[i] = (double *)malloc(sizeof(double) * 2);

}

MGAPlan::~MGAPlan() {
	free(m_solutions);
	for (int i = 0; i < max_pareto; ++i) free(m_pareto[i]);
	free(m_pareto);

	if (m_param) {
		orbiterkep__parameters__free_unpacked(m_param, NULL);
	}
}


void MGAPlan::ResetSolutions() {
	for (int i = 0; i < n_solutions; ++i) {
		orbiterkep__trans_xsolution__free_unpacked(m_solutions[i], NULL);
		m_solutions[i] = 0;
	}
	n_solutions = 0;
	m_best_solution = 0;
}

void MGAPlan::AddPareto(double x, double y)
{
	if (m_n_pareto < max_pareto) {
		int i = m_n_pareto;
		m_pareto[i][0] = x; m_pareto[i][1] = y;
		++m_n_pareto;
	}
}

std::string MGAPlan::get_solution_str(const Orbiterkep__TransXSolution & solution) const {
	char m_solution_buf[16000];
	if (m_best_solution == 0) return "";

	int len_sol = sprintf_transx_solution(m_solution_buf, m_best_solution);
	return std::string(m_solution_buf, len_sol);
}

void MGAPlan::AddSolution(Orbiterkep__TransXSolution * newSolution) {
	if (n_solutions < 25) {
		m_solutions[n_solutions] = newSolution;
		n_solutions += 1;
	}
	else {

		int min_idx = -1;
		double min = DBL_MAX;

		int max_idx = -1;
		double max = 0;
		for (int i = 0; i < n_solutions; ++i) {
			double fuelCost = m_solutions[i]->fuel_cost;
			if (fuelCost < min) {
				min_idx = i;
				min = fuelCost;
			}
			if (fuelCost > max) {
				max_idx = i;
				max = fuelCost;
			}
		}

		if (newSolution->fuel_cost > max) return;

		if (m_best_solution == m_solutions[max_idx]) {
			m_best_solution = 0;
		}
		orbiterkep__trans_xsolution__free_unpacked(m_solutions[max_idx], NULL);
		m_solutions[max_idx] = newSolution;
	}

	if (m_best_solution == 0 || newSolution->fuel_cost < m_best_solution->fuel_cost) {
		m_best_solution = newSolution;
	}
}