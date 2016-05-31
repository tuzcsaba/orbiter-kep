#include "mga_1dsm_transx.h"

#include <keplerian_toolbox/core_functions/array3D_operations.h>
#include <keplerian_toolbox/core_functions/propagate_lagrangian.h>
#include <keplerian_toolbox/core_functions/fb_prop.h>
#include <keplerian_toolbox/lambert_problem.h>

#include <string>
#include <cmath>
#include <numeric>
#include <vector>

#include <boost/array.hpp>
#include <boost/math/constants/constants.hpp>

namespace pagmo { namespace problem {

mga_1dsm_transx::mga_1dsm_transx(const std::vector<kep_toolbox::planet::planet_ptr> seq,
                    const double dep_altitude, const double arr_altitude, const bool circularize,
                    const kep_toolbox::epoch t0_l, const kep_toolbox::epoch t0_u,
                    const double tof_l, const double tof_u,
                    const double vinf_l, const double vinf_u,
                    const bool add_vinf_dep, const bool add_vinf_arr,
                    const bool multi_objective) : transx_problem(seq, dep_altitude, arr_altitude, circularize, 7 + (seq.size() - 2) * 4, 1 + (int)multi_objective), m_add_vinf_dep(add_vinf_dep), m_add_vinf_arr(add_vinf_arr) {
    size_t dim(get_dimension());
    decision_vector lb(dim, 0.0), ub(dim, 0.0);
    lb[0] = t0_l.mjd2000(); ub[0] = t0_u.mjd2000();
    lb[1] = tof_l * 365.25; ub[1] = tof_u * 365.25;

    lb[2] = lb[3] = 0.0; ub[2] = ub[3] = 1.0;
    lb[4] = vinf_l * 1000; ub[4] = vinf_u * 1000;
    lb[5] = lb[6] = 1e-5; ub[5] = ub[6] = 1 - 1e-5;

    for (int i = 0; i < get_n_legs() - 1; ++i) {
      lb[7 + 4 * i] = -2 * M_PI; ub[7 + 4 * i] = 2 * M_PI;
      lb[8 + 4 * i] = 1.1; ub[8 + 4 * i] = 30.0;
      lb[9 + 4 * i] = lb[10 + 4 * i] = 1e-5; ub[9 + 4 * i] = ub[10 + 4 * i] = 1-1e-5;
    }

    for (int i = 1; i < get_n_legs() - 1; ++i) {
      kep_toolbox::planet::planet_ptr pl = get_seq()[i];
      lb[8 + 4 * i] = pl->get_safe_radius() / pl->get_radius(); 
    }

    set_bounds(lb, ub);
}

mga_1dsm_transx::mga_1dsm_transx(const mga_1dsm_transx &p) : transx_problem(p.get_seq(), p.get_dep_altitude(), p.get_arr_altitude(), p.get_circularize(), p.get_dimension(), p.get_f_dimension()), m_add_vinf_arr(p.get_add_vinf_arr()), m_add_vinf_dep(p.get_add_vinf_dep()), m_multi_obj(p.get_multi_obj()) {
  set_bounds(p.get_lb(), p.get_ub());
}

base_ptr mga_1dsm_transx::clone() const {
  return base_ptr(new mga_1dsm_transx(*this));
}

void mga_1dsm_transx::calc_objective(fitness_vector &f, const decision_vector &x, bool should_print) const {
  std::vector<double> T(get_n_legs());
  double alpha_sum = 0;
  for (int i = 0; i < T.size(); ++i) {
    T[i] = - log(x[6 + 4 * i]);
    alpha_sum += T[i];
  }

  double theta = 2 * M_PI * x[2];
  double phi = acos(2 * x[3] - 1) - M_PI / 2;

  kep_toolbox::array3D Vinf;
  Vinf[0] = x[4] * cos(phi) * cos(theta);
  Vinf[1] = x[4] * cos(phi) * sin(theta);
  Vinf[2] = x[4] * sin(phi);

  for (int i = 0; i < T.size(); ++i) {
    T[i] = x[1] * T[i] / alpha_sum;
  }


  int n = get_seq().size();
  std::vector<kep_toolbox::epoch> t_P(n);
  std::vector<kep_toolbox::array3D> r_P(n);
  std::vector<kep_toolbox::array3D> v_P(n);
  std::vector<double> DV(n, 0.0);

  for (int i = 0; i < n; ++i) {
    kep_toolbox::planet::planet_ptr planet = get_seq()[i];
    t_P[i] = kep_toolbox::epoch(x[0] + std::accumulate(T.begin(), T.begin() + i, 0.0));
    planet->eph(t_P[i], r_P[i], v_P[i]);
  }

  if (should_print) {
    print_time_info(get_seq(), t_P);
  }

  if (m_add_vinf_dep) {
    DV[0] += burn_cost(get_seq()[0], Vinf, false, true); 
    if (should_print) {
      print_escape(get_seq()[0], v_P[0], r_P[0], Vinf, t_P[0].mjd());
    }
  }

  kep_toolbox::array3D v0(Vinf);
  kep_toolbox::diff(v0, v_P[0], Vinf);
  kep_toolbox::array3D r(r_P[0]), v(v0);
  kep_toolbox::propagate_lagrangian(r, v, x[5] * T[0] * ASTRO_DAY2SEC, get_common_mu());

  double dt = (1 - x[5]) * T[0] * ASTRO_DAY2SEC;
  kep_toolbox::lambert_problem l(r, r_P[1], dt, get_common_mu());
  kep_toolbox::array3D v_beg_l(l.get_v1()[0]);
  kep_toolbox::array3D v_end_l(l.get_v2()[0]);

  kep_toolbox::array3D deltaV(v_beg_l);
  kep_toolbox::diff(deltaV, v_beg_l, v);
  DV[0] += kep_toolbox::norm(deltaV);

  if (should_print) {
    print_dsm(v, r, deltaV, v_beg_l, t_P[0].mjd() + dt / ASTRO_DAY2SEC);
  }

  for (int i = 1; i < n - 1; ++i) {
    double radius = x[8 + (i - 1) * 4] * get_seq()[i]->get_radius();
    double beta = x[7 + (i - 1) * 4];
    kep_toolbox::array3D v_out;
    kep_toolbox::fb_prop(v_out, v_end_l, v_P[i], radius, beta, get_seq()[i]->get_mu_self());

    if (should_print) {
      kep_toolbox::array3D v_rel_in, v_rel_out;
      kep_toolbox::diff(v_rel_in, v_end_l, v_P[i]);
      kep_toolbox::diff(v_rel_out, v_out, v_P[i]);
      print_flyby(get_seq()[i], v_P[i], r_P[i], v_rel_in, v_rel_out, t_P[i].mjd());
    }

    r = r_P[i]; v = v_out;
    kep_toolbox::propagate_lagrangian(r, v, x[9 + (i - 1) * 4] * T[i] * ASTRO_DAY2SEC, get_common_mu());

    dt = (1 - x[9 + (i - 1) * 4]) * T[i] * ASTRO_DAY2SEC;
    kep_toolbox::lambert_problem l2(r, r_P[i + 1], dt, get_common_mu(), false, false);
    v_end_l = l2.get_v2()[0];
    v_beg_l = l2.get_v1()[0];

    kep_toolbox::diff(deltaV, v_beg_l, v);
    DV[i] = kep_toolbox::norm(deltaV);
    if (should_print) {
      print_dsm(v, r, deltaV, v_beg_l, t_P[i].mjd() + dt / ASTRO_DAY2SEC);
    }
  }

  if (m_add_vinf_arr) {
    kep_toolbox::array3D Vexc_arr(v_end_l);
    kep_toolbox::diff(Vexc_arr, v_end_l, v_P[v_P.size() - 1]);
    DV[DV.size() - 1] = burn_cost(get_seq()[get_seq().size() - 1], Vexc_arr, true, get_circularize());
    if (should_print) {
      print_arrival(get_seq()[get_seq().size() - 1], Vexc_arr, t_P[t_P.size() - 1].mjd());
    }
  }
  
  double sumDeltaV = std::accumulate(DV.begin(), DV.end(), 0.0);
  double sumT = std::accumulate(T.begin(), T.end(), 0.0);

  if (should_print) {
    std::cout << "Total fuel cost:    " << sumDeltaV << std::setprecision(3) << " m/s" << std::endl;
  }

  f[0] = sumDeltaV;
  if (get_f_dimension() == 2) {
    f[1] = sumT;
  }
}

}} // namespaces

BOOST_CLASS_EXPORT_IMPLEMENT(pagmo::problem::mga_1dsm_transx)