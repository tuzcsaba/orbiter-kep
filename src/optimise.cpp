
#include "optimise.h"

#include "plot/pareto.h"

namespace orbiterkep {

optimiser::optimiser(const pagmo::problem::base &prob, const int n_trial, const int gen, const int mf, const double mr) : 
  m_problem(prob), m_n_isl(8), m_population(60), m_n_trial(n_trial), m_gen(gen), m_mf(mf), m_mr(mr) {
}

pagmo::decision_vector optimiser::run_once(const pagmo::decision_vector *single_obj_result, const bool print_fronts, double maxDeltaV) {
  pagmo::algorithm::jde jde(m_mf, 2, 1, 1e-01, 1e-02, true);
  pagmo::algorithm::nsga2 nsga2(m_mf);


  pagmo::migration::best_s_policy sel_single(2, pagmo::migration::rate_type::absolute);
  pagmo::migration::fair_r_policy rep_single(2, pagmo::migration::rate_type::absolute);

  pagmo::migration::best_s_policy sel_multi(0.1, pagmo::migration::rate_type::fractional);
  pagmo::migration::fair_r_policy rep_multi(0.1, pagmo::migration::rate_type::fractional);

  pagmo::algorithm::base *algo = (pagmo::algorithm::base *)&jde;

  pagmo::migration::base_s_policy * sel = &sel_single;
  pagmo::migration::base_r_policy * rep = &rep_single;

  if (m_problem.get_f_dimension() == 2) {
    algo = (pagmo::algorithm::base *)&nsga2;

    sel = &sel_multi;
    rep = &rep_multi;
  }

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
        pagmo::island isl(*algo, pop, *sel, *rep);
        archi.push_back(isl);
        ++i;
      }
    }

    while (i < m_n_isl) {
      pagmo::island isl(*algo, m_problem, m_population, *sel, *rep);
      archi.push_back(isl);
      ++i;
    }

    double prev;
    for (int i = 0; i < m_gen / m_mf; ++i) {

      std::cout << "." << std::flush;
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
          std::cerr << "Improved: " << best_f << std::fixed << std::setprecision(3) << " m/s" << std::endl;
        }

        prev = val;
      }

    }

    if (m_problem.get_f_dimension() == 2 && print_fronts) {
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
