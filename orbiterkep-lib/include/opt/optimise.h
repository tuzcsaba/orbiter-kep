#include <pagmo/pagmo.h>

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

} // namespaces
