#include <pagmo/pagmo.h>

#include "proto/parameters.pb.h"
#include "proto/parameters.pb-c.h"

extern "C" {
#include "proto/solution.pb.h"
#include "proto/solution.pb-c.h"
}


namespace orbiterkep {

class optimiser {
   public:
     optimiser(const pagmo::problem::base &prob, const int n_trial, const int gen, const int mf, const double mr);

     pagmo::decision_vector run_once(pagmo::decision_vector *single_obj_result, const bool print_fronts = false, double maxDeltaV = 20000, std::vector<std::string> algo_list = std::vector<std::string>());

   private:
     const pagmo::problem::base &m_problem;
     int m_n_isl;
     int m_population;
     int m_n_trial;
     int m_mf;
     double m_mr;
     int m_gen;
};

void optimize(const Parameters &params, TransXSolution * solution);

} // namespaces


extern "C" _Orbiterkep__TransXSolution * orbiterkep_optimize(const _Orbiterkep__Parameters &params);
