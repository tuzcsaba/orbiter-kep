#include "OrbiterKEPConfig.h"

#include <string>
#include <iostream>

#include <pagmo/pagmo.h>

#include "model/param.h"
#include "problems/mga_transx.h"
#include "problems/mga_1dsm_transx.h"
#include "opt/optimise.h"

#ifdef BUILD_MONGODB

#include "db/orbiter_db.h"

#endif

namespace orbiterkep {
  

} // namespaces

void run_problem(pagmo::problem::TransXSolution * solution, const pagmo::problem::transx_problem &single_obj, const pagmo::problem::transx_problem &multi_obj, int trials, int gen, double max_deltav, bool run_multi_obj) {
      int mf = 150;

      pagmo::decision_vector sol_mga;

      std::cout << "- Running single-objective optimisation";
      orbiterkep::optimiser op(single_obj, trials, gen, mf, 1);
      sol_mga = op.run_once(0, false, max_deltav);
      std::cout << " Done" << std::endl;

      if (run_multi_obj) {
        std::cout << "- Running multi-objective optimisation";
        orbiterkep::optimiser op_multi(multi_obj, trials, gen, mf, 1);
        op_multi.run_once(&sol_mga, true, max_deltav);
        std::cout << "Done" << std::endl;
      }

      single_obj.fill_solution(solution, sol_mga);
}

int main(int argc, char **argv) {
  try {
    orbiterkep::orbiterkep_db db;

    orbiterkep::Parameters param;
    
    orbiterkep::parse_parameters(&param, argc, argv);

    std::cout << std::fixed;
    std::cerr << std::fixed;

    auto planets = orbiterkep::kep_toolbox_planets(param);

    if (param.problem() == "MGA" && param.n_trials() > 0) {
      std::cout << "Executing MGA trials" << std::endl;

      pagmo::problem::mga_transx mga(planets, 
          param.dep_altitude(), param.arr_altitude(), param.circularize(), 
          param.t0().min(), param.t0().max(), param.tof().min(), param.tof().max(), 
          param.vinf().min(), param.vinf().max(),
          param.add_dep_vinf(), param.add_arr_vinf(),
          false);

      pagmo::problem::mga_transx mga_multi(planets,
          param.dep_altitude(), param.arr_altitude(), param.circularize(), 
          param.t0().min(), param.t0().max(), param.tof().min(), param.tof().max(), 
          param.vinf().min(), param.vinf().max(),
          param.add_dep_vinf(), param.add_arr_vinf(),
           true);

      pagmo::problem::TransXSolution solution;
      if (param.use_db()) {
        mga.fill_solution(&solution, db.get_stored_solution(param, "MGA"));
      } else {
        run_problem(&solution, mga, mga_multi, param.n_trials(), param.n_gen(), param.max_deltav(), param.multi_objective());

        db.store_solution(param, solution, "MGA");
      }

      std::cout << solution << std::endl;

    }
    if (param.problem() == "MGA-1DSM" && param.n_trials() > 0) {
      std::cout << "Executing MGA-1DSM trials" << std::endl;

      pagmo::problem::mga_1dsm_transx mga_1dsm(planets,
          param.dep_altitude(), param.arr_altitude(), param.circularize(), 
          param.t0().min(), param.t0().max(), param.tof().min(), param.tof().max(), 
          param.vinf().min(), param.vinf().max(),
          param.add_dep_vinf(), param.add_arr_vinf(),
          false, true);


      pagmo::problem::mga_1dsm_transx mga_1dsm_multi(planets,
          param.dep_altitude(), param.arr_altitude(), param.circularize(), 
          param.t0().min(), param.t0().max(), param.tof().min(), param.tof().max(), 
          param.vinf().min(), param.vinf().max(),
          param.add_dep_vinf(), param.add_arr_vinf(),
          true, true);


      pagmo::problem::TransXSolution solution;
      if (param.use_db()) {
        mga_1dsm.fill_solution(&solution, db.get_stored_solution(param, "MGA-1DSM"));
      } else {
        run_problem(&solution, mga_1dsm, mga_1dsm_multi, param.n_trials(), param.n_gen(), param.max_deltav(), param.multi_objective());

        db.store_solution(param, solution, "MGA-1DSM");
      }

      std::cout << solution << std::endl;
    }
  } catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
  }
  return 0;
}
