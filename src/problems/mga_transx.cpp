#include <string>
#include <boost/math/constants/constants.hpp>
#include <vector>
#include <numeric>
#include <cmath>
#include <boost/array.hpp>
#include <keplerian_toolbox/core_functions/fb_vel.h>
#include <keplerian_toolbox/core_functions/array3D_operations.h>
#include <keplerian_toolbox/lambert_problem.h>

#include "mga_transx.h"

namespace pagmo { namespace problem {

mga_transx::mga_transx(const std::vector<kep_toolbox::planet::planet_ptr> seq,
    const double dep_altitude, const double arr_altitude, const bool circularize,
    const kep_toolbox::epoch t0_l, const kep_toolbox::epoch t0_u,
    const double tof_l, const double tof_u, 
    const double vinf_l, const double vinf_u, 
    const bool add_vinf_dep, const bool add_vinf_arr,
    const bool multi_obj) : transx_problem(seq, dep_altitude, arr_altitude, circularize, seq.size() + 1, 1 + (int)multi_obj), m_add_vinf_dep(add_vinf_dep), m_add_vinf_arr(add_vinf_arr), m_multi_obj(multi_obj) {
    
        size_t dim(get_dimension());
        decision_vector lb(dim), ub(dim);
        lb[0] = t0_l.mjd2000(); ub[0] = t0_u.mjd2000();
        lb[1] = tof_l * 365.25; ub[1] = tof_u * 365.25;

        for (int i = 0; i < dim - 2; ++i) {
          lb[2 + i] = 1e-5; ub[2 + i] = 1 - 1e-5; 
        }

        set_bounds(lb, ub);
}

mga_transx::mga_transx(const mga_transx &p) : transx_problem(p.get_seq(), p.get_dep_altitude(), p.get_arr_altitude(), p.get_circularize(), p.get_dimension(), p.get_f_dimension()), m_add_vinf_dep(p.m_add_vinf_dep), m_add_vinf_arr(p.m_add_vinf_arr), m_multi_obj(p.m_multi_obj) {
  set_bounds(p.get_lb(), p.get_ub());

}

base_ptr mga_transx::clone() const {
  return base_ptr(new mga_transx(*this));
}

TransXSolution mga_transx::calc_objective(fitness_vector &f, const decision_vector &x, bool should_print) const {

  TransXSolution solution;

  int n = get_n_legs();

  std::vector<double> T(n, 0.0);

  double alpha_sum = 0.0;
  for (int i = 0; i < get_n_legs(); ++i) {
    double tmp = -log(x[2 + i]);
    alpha_sum += tmp;
    T[i] = x[1] * tmp;
  }

  for (int i = 0; i < T.size(); ++i) {
    T[i] /= alpha_sum;
  }

  std::vector<kep_toolbox::epoch>   t_P(get_seq().size());
  std::vector<kep_toolbox::array3D> r_P(get_seq().size());
  std::vector<kep_toolbox::array3D> v_P(get_seq().size());
  std::vector<double> DV(get_seq().size());
  for (int i = 0; i < get_seq().size(); ++i) {
    kep_toolbox::planet::planet_ptr planet = get_seq()[i];
    t_P[i] = kep_toolbox::epoch(x[0] + std::accumulate(T.begin(), T.begin()+i, 0.0));
    planet->eph(t_P[i], r_P[i], v_P[i]);
  }

  kep_toolbox::array3D r(r_P[0]), v(v_P[0]);
  kep_toolbox::array3D v_end_l;
  kep_toolbox::array3D v_beg_l;

  transx_time_info(solution.mutable_times(), get_seq(), t_P);

  kep_toolbox::array3D vout, vin;
  for (int i = 0; i < get_n_legs(); ++i) {
    r = r_P[i]; v = v_P[i];

    double dt = (t_P[i + 1].mjd() - t_P[i].mjd()) * ASTRO_DAY2SEC;
    kep_toolbox::lambert_problem l(r, r_P[i + 1], dt, get_common_mu(), false, false);
    v_beg_l = l.get_v1()[0];
    v_end_l = l.get_v2()[0];

    vout = v_beg_l;

    if (i == 0) {
      kep_toolbox::array3D vout_rel(vout);
      kep_toolbox::diff(vout_rel, vout, v_P[i]);
      if (get_add_vinf_dep()) {
        DV[0] = burn_cost(get_seq()[0], vout_rel, false, true);
      }
      if (should_print) {
        transx_escape(solution.mutable_escape(), get_seq()[0], v_P[0], r_P[0], vout_rel, t_P[0].mjd());
      }
    } else {
      kep_toolbox::array3D v_rel_in(vin), v_rel_out(vout);
      kep_toolbox::diff(v_rel_in, vin, v_P[i]);
      kep_toolbox::diff(v_rel_out, vout, v_P[i]);
      kep_toolbox::planet::planet_ptr planet = get_seq()[i];
      kep_toolbox::fb_vel(DV[i], v_rel_in, v_rel_out, *planet);

      if (should_print) {
        transx_flyby(solution.add_flybyes(), planet, v_P[i], r_P[i], v_rel_in, v_rel_out, t_P[i].mjd());
      }
    }

    vin = v_end_l;
  }

  kep_toolbox::array3D Vexc_arr;
  kep_toolbox::diff(Vexc_arr, v_end_l, v_P[v_P.size() - 1]);
  if (get_add_vinf_arr()) {
    DV[DV.size() - 1] = burn_cost(get_seq()[get_seq().size() - 1], Vexc_arr, true, get_circularize());
  }

  if (should_print)
    transx_arrival(solution.mutable_arrival(), get_seq()[get_seq().size() - 1], Vexc_arr, t_P[t_P.size() - 1].mjd());

  double fuelCost = std::accumulate(DV.begin(), DV.end(), 0.0);
  double totalTime = std::accumulate(T.begin(), T.end(), 0.0);

  if (should_print) {
    solution.set_fuel_cost(fuelCost);
  }

  f[0] = fuelCost;
  if (get_f_dimension() == 2) {
    f[1] = totalTime;
  }

  return solution;
}

std::string mga_transx::get_name() const {
  return "MGA";
}

}} // namespaces


BOOST_CLASS_EXPORT_IMPLEMENT(pagmo::problem::mga_transx)
