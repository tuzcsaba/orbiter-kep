#include "proto/ext.h"

#include <pagmo/problem/base.h>

#include "orbiterkep-lib_Export.h"

namespace pagmo { namespace algorithm {

    class base;

}} // namespaces

namespace orbiterkep {

	class Parameters;
	class TransXSolution;

class optimiser {
   public:
     optimiser(const pagmo::problem::base &prob, const int n_isl = 4, const int population = 60, const int gen = 10000, const int mf = 100, const double mr = 0.15);
     ~optimiser();

     pagmo::decision_vector run_once(pagmo::decision_vector *single_obj_result, const Parameters &params, std::vector<std::pair<double, double> > &pareto);

     static orbiterkep_lib_EXPORT void __cdecl optimize(const Parameters &params, TransXSolution * solution, std::vector<std::pair<double, double> > &pareto);

   private:
     std::map<std::string, pagmo::algorithm::base *> m_algos_map;

     const pagmo::problem::base &m_problem;
     int m_n_isl;
     int m_population;
     int m_mf;
     double m_mr;
     int m_gen;
};

} // namespaces
