
#include "optimise.h"

#include "plot/pareto.h"

namespace orbiterkep {

optimiser::optimiser(const pagmo::problem::base &prob, const int n_trial, const int gen, const int mf, const double mr) : 
  m_problem(prob), m_n_isl(8), m_population(60), m_n_trial(n_trial), m_mf(mf), m_gen(gen), m_mr(mr) {
}

pagmo::decision_vector optimiser::run_once(pagmo::decision_vector *single_obj_result, const bool print_fronts, double maxDeltaV, std::vector<std::string> algo_list) {

  std::map<std::string, pagmo::algorithm::base_ptr> algos_map;

  // Single-obj
  algos_map["jde"] = std::unique_ptr<pagmo::algorithm::base>(new pagmo::algorithm::jde(m_mf, 2, 1, 1e-1, 1e-2, true));
  algos_map["jde_11"] = std::unique_ptr<pagmo::algorithm::base>(new pagmo::algorithm::jde(m_mf, 11, 2, 1e-1, 1e-2, true));
  algos_map["jde_13"] = std::unique_ptr<pagmo::algorithm::base>(new pagmo::algorithm::jde(m_mf, 13, 2, 1e-1, 1e-2, true));
  algos_map["jde_15"] = std::unique_ptr<pagmo::algorithm::base>(new pagmo::algorithm::jde(m_mf, 15, 2, 1e-1, 1e-2, true));
  algos_map["jde_17"] = std::unique_ptr<pagmo::algorithm::base>(new pagmo::algorithm::jde(m_mf, 17, 2, 1e-1, 1e-2, true));
  algos_map["de_1220"] = std::unique_ptr<pagmo::algorithm::base>(new pagmo::algorithm::de_1220(m_mf));
  algos_map["mde_pbx"] = std::unique_ptr<pagmo::algorithm::base>(new pagmo::algorithm::mde_pbx(m_mf));
  algos_map["pso"] = std::unique_ptr<pagmo::algorithm::base>(new pagmo::algorithm::pso(m_mf));
  algos_map["bee_colony"] = std::unique_ptr<pagmo::algorithm::base>(new pagmo::algorithm::bee_colony(m_mf));
  algos_map["cmaes"] = std::unique_ptr<pagmo::algorithm::base>(new pagmo::algorithm::cmaes(m_mf));
  algos_map["sga_gray"] = std::unique_ptr<pagmo::algorithm::base>(new pagmo::algorithm::sga_gray(m_mf));
  algos_map["sa_corana"] = std::unique_ptr<pagmo::algorithm::base>(new pagmo::algorithm::sa_corana(m_mf * 100));

  // Multi-obj
  algos_map["nsga2"] = std::unique_ptr<pagmo::algorithm::base>(new pagmo::algorithm::nsga2(m_mf));

  // Meta-algorithm
  
  // Local optimizers
  algos_map["cs"] = std::unique_ptr<pagmo::algorithm::base>(new pagmo::algorithm::cs(m_mf));
 
  // Meta-algorithm
  algos_map["mbh_cs"] = std::unique_ptr<pagmo::algorithm::base>(new pagmo::algorithm::mbh(*algos_map["cs"]));
  algos_map["ms_jde"] = std::unique_ptr<pagmo::algorithm::base>(new pagmo::algorithm::ms(*algos_map["jde"], m_mf));

  pagmo::migration::best_s_policy sel_single(0.1, pagmo::migration::rate_type::fractional);
  pagmo::migration::fair_r_policy rep_single(0.1, pagmo::migration::rate_type::fractional);

  pagmo::migration::best_s_policy sel_multi(0.1, pagmo::migration::rate_type::fractional);
  pagmo::migration::fair_r_policy rep_multi(0.1, pagmo::migration::rate_type::fractional);

  std::vector<pagmo::algorithm::base_ptr> algos;

  pagmo::migration::base_s_policy * sel = &sel_single;
  pagmo::migration::base_r_policy * rep = &rep_single;

  if (m_problem.get_f_dimension() == 2) {
    sel = &sel_multi;
    rep = &rep_multi;
  }

  if (algo_list.size() == 0) {
    algos.push_back((m_problem.get_f_dimension() == 2 ? algos_map["nsga2"] : algos_map["jde"]));
  } else {
    for (int i = 0; i < algo_list.size(); ++i) {
      algos.push_back(algos_map[algo_list[i]]);
    }
  }

  const pagmo::topology::ageing_clustered_ba topology = pagmo::topology::ageing_clustered_ba();

  double best_f = 10000000;
  pagmo::decision_vector best_x;

  pagmo::decision_vector *to_feed = single_obj_result;

  for (int u = 0; u < m_n_trial; ++u) {
    pagmo::archipelago archi(topology);
    int i = 0;

    if (to_feed!= 0) {
      pagmo::population pop(m_problem, m_population - 1);
      pop.push_back(*to_feed);
      pagmo::island isl(*algos[i % algos.size()], pop, *sel, *rep);
      archi.push_back(isl);
      ++i;
    }

    while (i < m_n_isl) {
      pagmo::island isl(*algos[i % algos.size()], m_problem, m_population, *sel, *rep);
      archi.push_back(isl);
      ++i;
    }

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
