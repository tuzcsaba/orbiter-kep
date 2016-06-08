#include "OrbiterKEPConfig.h"

#include <string>
#include <iostream>

#include "model/param.h"
#include "opt/optimise.h"
#include "opt/optimise-c.h"

#include "proto/parameters.pb-c.h"
#include "proto/solution.pb-c.h"

#include "proto/ext.h"

int main(int argc, char **argv) {

    orbiterkep::Parameters par;

    if (!orbiterkep::parse_parameters(&par, argc, argv)) {
      return -1;
    }

    orbiterkep::TransXSolution sol;
    orbiterkep::optimiser::optimize(par, &sol);

    std::cout << sol.fuel_cost() << std::endl;

    std::cout << std::fixed;
    std::cerr << std::fixed;

    Orbiterkep__Parameters param = ORBITERKEP__PARAMETERS__INIT;
	char * planets[] = { "earth", "venus", "mercury" };
	param.n_planets = 3;
	param.planets = planets;
	char * single_algos[] = { "jde" };
	param.n_single_objective_algos = 1;
	param.single_objective_algos = single_algos;
	char * multi_algos[] = { "nsga2" };
	param.n_multi_objective_algos = 1;
	param.multi_objective_algos = multi_algos;

	param.problem = "MGA";

	param.has_n_trials = 1;
	param.n_trials = 1;

	param.has_n_gen = 1;
	param.n_gen = 100000;

	Orbiterkep__ParamBounds t0 = ORBITERKEP__PARAM_BOUNDS__INIT;
	t0.has_lb = 1; t0.has_ub = 1;
	t0.lb = 51000; t0.ub = 56000;
	param.t0 = &t0;

	Orbiterkep__ParamBounds tof = ORBITERKEP__PARAM_BOUNDS__INIT;
	tof.has_lb = 1; tof.has_ub = 1;
	tof.lb = 0.1; tof.ub = 5.0;
	param.tof = &tof;

	Orbiterkep__ParamBounds vinf = ORBITERKEP__PARAM_BOUNDS__INIT;
	vinf.has_lb = 1; vinf.has_ub = 1;
	vinf.lb = 0.1; vinf.ub = 12.0;
	param.vinf = &vinf;

	param.has_max_deltav = 1;
	param.max_deltav = 24.0;

	param.has_circularize = 1;
	param.circularize = 1;

	param.has_add_arr_vinf = 1;
	param.add_arr_vinf = 1;

	param.has_add_dep_vinf = 1;
	param.add_dep_vinf = 1;

	param.has_dep_altitude = 1;
	param.dep_altitude = 300;

	param.has_arr_altitude = 1;
	param.arr_altitude = 300;

	param.has_multi_objective = 1;
	param.multi_objective = 0;

	param.has_use_db = 1;
	param.use_db = 0;

	param.has_use_spice = 1;
	param.use_spice = 0;

	int len = orbiterkep__parameters__get_packed_size(&param);
	void * buf = malloc(len);
	orbiterkep__parameters__pack(&param, (uint8_t *)buf);

	void * sol_buf = malloc(2000);
	int sol_len = orbiterkep_optimize((const uint8_t *)buf, len, (uint8_t *)sol_buf);
	Orbiterkep__TransXSolution * solution = orbiterkep__trans_xsolution__unpack(NULL, sol_len, (uint8_t *)sol_buf);

    std::cout << solution->fuel_cost << std::endl;


	orbiterkep__trans_xsolution__free_unpacked(solution, NULL);

  return 0;
}
