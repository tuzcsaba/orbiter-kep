#ifndef GRAVITY_ASSIST_MGA_PLAN_H
#define GRAVITY_ASSIST_MGA_PLAN_H

#include "orbiterkep/opt/optimise-c.h"
#include "orbiterkep/proto/solution.pb-c.h"
#include "orbiterkep/proto/parameters.pb-c.h"

#include <vector>

Orbiterkep__Parameters * newDefaultParameters();

class MGAPlan {
public:
	MGAPlan(Orbiterkep__Parameters * param);
	~MGAPlan();

	Orbiterkep__Parameters & param() const { return *m_param; };
	void UpdateParam(Orbiterkep__Parameters *param) {
		orbiterkep__parameters__free_unpacked(m_param, NULL);
		m_param = param;
	}

	/** Single-objective */
	bool has_solution() const { return n_solutions > 0; }
	int get_n_solutions() const { return n_solutions; }
	const Orbiterkep__TransXSolution &get_best_solution() const { return *m_best_solution; };
	const Orbiterkep__TransXSolution &get_solution(int i) const { return *m_solutions[i]; };
	std::string get_solution_str(const Orbiterkep__TransXSolution & sol) const;

	/** Multi-objective */
	double ** pareto_buffer() { return m_pareto; }
	int * get_n_pareto() { return &m_n_pareto; }
	void get_pareto(int i, double &x, double &y) { x = m_pareto[i][0]; y = m_pareto[i][1]; };

	/** State */

	bool converged() const {
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

	void AddSolution(Orbiterkep__TransXSolution * newSolution);
	void AddPareto(double x, double y);
	void ResetSolutions();

private:
	Orbiterkep__Parameters * m_param;

	int n_solutions;
	Orbiterkep__TransXSolution ** m_solutions;
	Orbiterkep__TransXSolution * m_best_solution;

	const int max_pareto = 10000;
	double** m_pareto;
	int m_n_pareto;
};

#endif