#include "OrbiterKEPConfig.h"

#include <pagmo/pagmo.h>

#include "opt/optimise.h"

#include "model/param.h"

#include "plot/pareto.h"

#include "problems/mga_transx.h"
#include "problems/mga_1dsm_transx.h"

#ifdef BUILD_MONGODB

#include "db/orbiter_db.h"

#endif


namespace orbiterkep {

void run_problem(TransXSolution * solution, const pagmo::problem::transx_problem &single_obj, const pagmo::problem::transx_problem &multi_obj, const orbiterkep::Parameters &params, std::vector<std::pair<double, double> > &pareto) {
      int mf = params.pagmo().mf();
      double mr = params.pagmo().mr();
      int gen = params.pagmo().n_gen();
      int population = params.pagmo().population();
      int n_isl = params.pagmo().n_isl();

      pagmo::decision_vector sol_mga;

      if (params.multi_objective()) {
        std::cout << "- Running multi-objective optimisation";
        orbiterkep::optimiser op_multi(multi_obj, n_isl, population, gen, mf, mr);
        op_multi.run_once(0, params, pareto);
        std::cout << "Done" << std::endl;
      } else {
        std::cout << "- Running single-objective optimisation";
        orbiterkep::optimiser op(single_obj, n_isl, population, gen, mf, mr);
        sol_mga = op.run_once(0, params, pareto);
        std::cout << " Done" << std::endl;

        single_obj.fill_solution(solution, sol_mga);
      }
}

void optimiser::optimize(const Parameters &param, TransXSolution * solution, std::vector<std::pair<double, double> > &pareto) {
#ifdef BUILD_MONGODB
    orbiterkep::orbiterkep_db db;
#endif

    auto planets = orbiterkep::CommandLine::kep_toolbox_planets(param);

    pagmo::problem::transx_problem * single;
    pagmo::problem::transx_problem * multi;

    auto MJD = kep_toolbox::epoch::type::MJD;

    if (param.problem() == "MGA") {
      single = new pagmo::problem::mga_transx(planets,
          param.dep_altitude(), param.arr_altitude(), param.circularize(),
          kep_toolbox::epoch(param.t0().lb(), MJD), kep_toolbox::epoch(param.t0().ub(), MJD),
          param.tof().lb(), param.tof().ub(),
          param.vinf().lb(), param.vinf().ub(),
          param.add_dep_vinf(), param.add_arr_vinf(),
          false);

      multi = new pagmo::problem::mga_transx(planets,
          param.dep_altitude(), param.arr_altitude(), param.circularize(),
          kep_toolbox::epoch(param.t0().lb(), MJD), kep_toolbox::epoch(param.t0().ub(), MJD),
          param.tof().lb(), param.tof().ub(),
          param.vinf().lb(), param.vinf().ub(),
          param.add_dep_vinf(), param.add_arr_vinf(),
           true);

      std::cout << "Executing MGA trials" << std::endl;
    } else if (param.problem() == "MGA-1DSM") {
       single = new pagmo::problem::mga_1dsm_transx(planets,
          param.dep_altitude(), param.arr_altitude(), param.circularize(),
          kep_toolbox::epoch(param.t0().lb(), MJD), kep_toolbox::epoch(param.t0().ub(), MJD),
          param.tof().lb(), param.tof().ub(),
          param.vinf().lb(), param.vinf().ub(),
          param.add_dep_vinf(), param.add_arr_vinf(),
          false, true);

      multi = new pagmo::problem::mga_1dsm_transx(planets,
          param.dep_altitude(), param.arr_altitude(), param.circularize(),
          kep_toolbox::epoch(param.t0().lb(), MJD), kep_toolbox::epoch(param.t0().ub(), MJD),
          param.tof().lb(), param.tof().ub(),
          param.vinf().lb(), param.vinf().ub(),
          param.add_dep_vinf(), param.add_arr_vinf(),
           true, true);


    }


#ifdef BUILD_MONGODB
    if (param.use_db()) {
      single->fill_solution(solution, db.get_stored_solution(param, "MGA"));
    } else {
#endif
      run_problem(solution, *single, *multi, param, pareto);
#ifdef BUILD_MONGODB
      db.store_solution(param, *solution, "MGA");
    }
#endif

    delete single;
    delete multi;
}


optimiser::optimiser(const pagmo::problem::base &prob, const int n_isl, const int population, const int gen, const int mf, const double mr) :
  m_problem(prob), m_n_isl(n_isl), m_population(population), m_mf(mf), m_gen(gen), m_mr(mr) {
// Single-obj
  m_algos_map["jde"] = new pagmo::algorithm::jde(m_mf, 2, 1, 1e-1, 1e-2, true);
  m_algos_map["jde_11"] = new pagmo::algorithm::jde(m_mf, 11, 2, 1e-1, 1e-2, true);
  m_algos_map["jde_13"] = new pagmo::algorithm::jde(m_mf, 13, 2, 1e-1, 1e-2, true);
  m_algos_map["jde_15"] = new pagmo::algorithm::jde(m_mf, 15, 2, 1e-1, 1e-2, true);
  m_algos_map["jde_17"] = new pagmo::algorithm::jde(m_mf, 17, 2, 1e-1, 1e-2, true);
  m_algos_map["de_1220"] = new pagmo::algorithm::de_1220(m_mf);
  m_algos_map["mde_pbx"] = new pagmo::algorithm::mde_pbx(m_mf);
  m_algos_map["pso"] = new pagmo::algorithm::pso(m_mf);
  m_algos_map["bee_colony"] = new pagmo::algorithm::bee_colony(m_mf);
  m_algos_map["cmaes"] = new pagmo::algorithm::cmaes(m_mf);
  m_algos_map["sga_gray"] = new pagmo::algorithm::sga_gray(m_mf);
  m_algos_map["sa_corana"] = new pagmo::algorithm::sa_corana(m_mf * 100);

  // Multi-obj
  m_algos_map["nsga2"] = new pagmo::algorithm::nsga2(m_mf);

  // Meta-algorithm

  // Local optimizers
  m_algos_map["cs"] = new pagmo::algorithm::cs(m_mf);

  // Meta-algorithm
  m_algos_map["mbh_cs"] = new pagmo::algorithm::mbh(*m_algos_map["cs"]);
  m_algos_map["ms_jde"] = new pagmo::algorithm::ms(*m_algos_map["jde"], m_mf);
}

optimiser::~optimiser() {
    for(auto iterator = m_algos_map.begin(); iterator != m_algos_map.end(); iterator++) {
        delete iterator->second;
    }
}

void population_obtain_pareto_fronts(const pagmo::population &pop, double maxDeltaV, std::vector<std::pair<double, double> > &result) {

  std::vector<std::vector<pagmo::population::size_type> > p_list = pop.compute_pareto_fronts();

  double min_x = DBL_MAX, max_x = 0;
  double min_y = DBL_MAX, max_y = 0;
  for (int i = 0; i < p_list.size(); ++i) {
    std::vector<pagmo::population::size_type> f = p_list[i];

    for (int j = 0; j < p_list[i].size(); ++j) {
      double x = pop.get_individual(f[j]).cur_f[0];
      double y = pop.get_individual(f[j]).cur_f[1];
      if (maxDeltaV != -1 && x > maxDeltaV) continue;

      result.push_back(std::pair<double, double>(x, y));
    }
  }

}

pagmo::decision_vector optimiser::run_once(pagmo::decision_vector *single_obj_result, const Parameters &params, std::vector<std::pair<double, double> > &pareto) {

  pagmo::migration::best_s_policy sel_single(m_mr, pagmo::migration::rate_type::fractional);
  pagmo::migration::fair_r_policy rep_single(m_mr, pagmo::migration::rate_type::fractional);

  pagmo::migration::best_s_policy sel_multi(m_mr, pagmo::migration::rate_type::fractional);
  pagmo::migration::fair_r_policy rep_multi(m_mr, pagmo::migration::rate_type::fractional);

  std::vector<pagmo::algorithm::base *> algos;

  pagmo::migration::base_s_policy * sel = &sel_single;
  pagmo::migration::base_r_policy * rep = &rep_single;

  if (m_problem.get_f_dimension() == 2) {
    sel = &sel_multi;
    rep = &rep_multi;
  }

  auto theAlgos = params.multi_objective() ? params.multi_objective_algos() : params.single_objective_algos();
  std::vector<std::string> algo_list;
  for (auto algo : theAlgos) {
    algo_list.push_back(algo);
  }
  if (algo_list.size() == 0) {
    algos.push_back((m_problem.get_f_dimension() == 2 ? m_algos_map["nsga2"] : m_algos_map["jde"]));
  } else {
    for (int i = 0; i < algo_list.size(); ++i) {
      algos.push_back(m_algos_map[algo_list[i]]);
    }
  }

  const pagmo::topology::ageing_clustered_ba topology = pagmo::topology::ageing_clustered_ba();

  double best_f = 10000000;
  pagmo::decision_vector best_x;

  pagmo::decision_vector *to_feed = single_obj_result;

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

    bool print_fronts = params.multi_objective();
    if (m_problem.get_f_dimension() == 2) {
      pagmo::population sum_pop(m_problem);
      for (int i = 0; i < archi.get_size(); ++i) {
        pagmo::base_island_ptr isl = archi.get_island(i);
        for (int j = 0; j < isl->get_population().size(); ++j) {
          sum_pop.push_back(isl->get_population().get_individual(j).cur_x);
        }
      }

      population_obtain_pareto_fronts(sum_pop, params.max_deltav(), pareto);
#ifdef BUILD_PLOT
    if (print_fronts) {
      population_plot_pareto_fronts(sum_pop, params.max_deltav());
    }
#endif
    }
  }

  return best_x;
}

} // namespaces
