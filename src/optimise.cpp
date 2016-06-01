
#include "optimise.h"

#include "plot/pareto.h"

namespace orbiterkep {

optimiser::optimiser(pagmo::problem::base &prob, const int n_trial, const int gen, const int mf, const double mr) : 
  m_problem(prob), m_n_isl(8), m_population(60), m_n_trial(n_trial), m_gen(gen), m_mf(mf), m_mr(mr) {
}

pagmo::decision_vector optimiser::run_once(const pagmo::decision_vector *single_obj_result, const bool print_fronts, double maxDeltaV) {
  pagmo::algorithm::jde jde(m_mf, 2, 1, 1e-01, 1e-02, true);
  pagmo::algorithm::nsga2 nsga2(m_mf);

  pagmo::algorithm::base *algo = (pagmo::algorithm::base *)&jde;
  if (m_problem.get_f_dimension() == 2) {
    algo = (pagmo::algorithm::base *)&nsga2;
  }

  const pagmo::migration::base_s_policy &selection = pagmo::migration::best_s_policy(2, pagmo::migration::rate_type::absolute);
  const pagmo::migration::base_r_policy &replacement = pagmo::migration::fair_r_policy(2, pagmo::migration::rate_type::absolute);

  const pagmo::topology::fully_connected &topology = pagmo::topology::fully_connected();

  double best_f = 10000000;
  pagmo::decision_vector best_x;

  for (int u = 0; u < m_n_trial; ++u) {
    pagmo::archipelago archi(topology);
    int i = 0;
    if (m_problem.get_f_dimension() == 2) {
      if (single_obj_result != 0) {
        pagmo::population pop(m_problem, m_population - 1);
        pop.push_back(*single_obj_result);
        pagmo::island isl(*algo, pop, selection, replacement);
        archi.push_back(isl);
        ++i;
      }
    }


    while (i < m_n_isl) {
      pagmo::island isl(*algo, m_problem, m_population, selection, replacement);
      archi.push_back(isl);
      ++i;
    }


    double prev;

    for (int i = 0; i < m_gen / m_mf; ++i) {
      archi.evolve(1);
      archi.join();

      if (m_problem.get_f_dimension() == 1) {
        pagmo::base_island_ptr isl;

        for (int j = 0; j < archi.get_size(); ++j) {
          if (isl == 0 || isl->get_population().champion().f[0] > archi.get_island(j)->get_population().champion().f[0]) {
            isl = archi.get_island(j);
          }
        }

        double val = isl->get_population().champion().f[0];

        if (best_f > val) {
          best_f = val;
          best_x = isl->get_population().champion().x;
          std::cout << "Improved: " << best_f << std::fixed << std::setprecision(3) << " m/s" << std::endl;
        }

        prev = val;
      }

    }
    if (m_problem.get_f_dimension() == 2) {
      pagmo::population sum_pop(m_problem);
      for (int i = 0; i < archi.get_size(); ++i) {
        pagmo::base_island_ptr isl = archi.get_island(i);
        for (int j = 0; j < isl->get_population().size(); ++j) {
          sum_pop.push_back(isl->get_population().get_individual(j).cur_x);
        }
      }

      population_plot_pareto_fronts(sum_pop, maxDeltaV);
    }
    
  }

  return best_x;
}

} // namespaces
