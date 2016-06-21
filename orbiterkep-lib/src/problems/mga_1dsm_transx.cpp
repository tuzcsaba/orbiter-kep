#include "problems/mga_1dsm_transx.h"

#include <keplerian_toolbox/core_functions/array3D_operations.h>
#include <keplerian_toolbox/core_functions/fb_vel.h>
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

int dimension_from_params(const std::vector<kep_toolbox::planet::planet_ptr> seq, const std::vector<bool> m_dsm) {
    int dim = 2;
    int n = seq.size();
    bool dsm = m_dsm.size() == 0 || (m_dsm.size() > 0 && m_dsm[0]);
    dim += (dsm ? 4 : 1);
    for (int i = 1; i < seq.size() - 1; ++i) {
        dsm = m_dsm.size() == 0 || (m_dsm.size() > i && m_dsm[i]);
        dim += (dsm ? 4 : 1);
    }
    return dim;
}

bool mga_1dsm_transx::dsm_in_leg_i(int i) const {
    return (m_dsm.size() == 0) || (m_dsm.size() > i && m_dsm[i]);
}

int mga_1dsm_transx::t0_index() const { return 0; }
int mga_1dsm_transx::tof_index() const { return 1; }
int mga_1dsm_transx::vinf_index() const { return 3; } // 3, 4

int mga_1dsm_transx::base_idx(int i) const{
    bool dsm = dsm_in_leg_i(0);
    int base = (tof_index() + 1);
    if (i == 0) return base;

    int idx = (base + (dsm ? 4 : 1));
    for (int j = 1; j < i; ++j) {
        dsm = dsm_in_leg_i(j);
        idx += (dsm ? 4 : 1);
    }
    return idx;
}
int mga_1dsm_transx::T_idx(int i) const {
    int base = base_idx(i);
    if (i == 0) return base;

    return base;
}

int mga_1dsm_transx::Beta_idx(int i) const {
    int base = base_idx(i);
    return base + 1;
}

int mga_1dsm_transx::R_idx(int i) const {
    int base = base_idx(i);
    return base + 2;
}

int mga_1dsm_transx::DSM_idx(int i) const {
    int base = base_idx(i);
    if (i == 0) return base + 4;

    return base + 3;
}

mga_1dsm_transx::mga_1dsm_transx(const std::vector<kep_toolbox::planet::planet_ptr> seq,
                    const std::vector<bool> dsm,
                    const double dep_altitude, const double arr_altitude, const bool circularize,
                    const kep_toolbox::epoch t0_l, const kep_toolbox::epoch t0_u,
                    const double tof_l, const double tof_u,
                    const double vinf_l, const double vinf_u,
                    const bool add_vinf_dep, const bool add_vinf_arr,
                    const bool multi_objective) : transx_problem(seq, dep_altitude, arr_altitude, circularize, dimension_from_params(seq, dsm), 1 + (int)multi_objective), m_add_vinf_dep(add_vinf_dep), m_add_vinf_arr(add_vinf_arr), m_dsm(dsm) {
    size_t dim(get_dimension());
    decision_vector lb(dim, 0.0), ub(dim, 0.0);

      lb[0] = t0_l.mjd2000(); ub[0] = t0_u.mjd2000();
      lb[1] = tof_l * 365.25; ub[1] = tof_u * 365.25;

      lb[2] = 1e-5; ub[2] = 1 - 1e-5;

      if (dsm_in_leg_i(0)) {
        lb[vinf_index()] = lb[vinf_index() + 1] = 0.0; ub[vinf_index()] = ub[vinf_index() + 1] = 1.0;
        lb[vinf_index() + 2] = vinf_l * 1000; ub[vinf_index() + 2] = vinf_u * 1000;
        lb[DSM_idx(0)] = 1e-5; ub[DSM_idx(0)] = 1 - 1e-5;
      }

      for (int i = 0; i < get_n_legs() - 1; ++i) {
        auto planet = seq[i + 1];

        int j = base_idx(i + 1);
        bool dsm = dsm_in_leg_i(i + 1);
        lb[j] = 1e-5;  ub[j] = 1 - 1e-5; // T[i]
        if (dsm) {

            double a = planet->compute_elements()[0];
            double soi = a * pow((planet->get_mu_self() / planet->get_mu_central_body()), 2/5);
            double soiRad = soi / planet->get_radius();

            double safeDistanceRatio = planet->get_safe_radius() / planet->get_radius();

            lb[j + 1] = -2 * boost::math::constants::pi<double>(); ub[j + 1] = 2 * boost::math::constants::pi<double>(); // Beta
            lb[j + 2] = safeDistanceRatio;   ub[j + 2] = soiRad; // Rad
            lb[j + 3] = 1e-5;  ub[j + 3] = 1 - 1e-5;
        }
      }

        for (int i = 1; i < get_n_legs() - 1; ++i) {
            bool dsm = dsm_in_leg_i(i);
            if (dsm) {
                kep_toolbox::planet::planet_ptr pl = get_seq()[i];
                lb[R_idx(i)] = pl->get_safe_radius() / pl->get_radius();
            }
        }

    set_bounds(lb, ub);
}

mga_1dsm_transx::mga_1dsm_transx(const mga_1dsm_transx &p) : transx_problem(p.get_seq(), p.get_dep_altitude(), p.get_arr_altitude(), p.get_circularize(), p.get_dimension(), p.get_f_dimension()), m_dsm(p.m_dsm), m_add_vinf_arr(p.get_add_vinf_arr()), m_add_vinf_dep(p.get_add_vinf_dep()), m_multi_obj(p.get_multi_obj()) {
  set_bounds(p.get_lb(), p.get_ub());
}

base_ptr mga_1dsm_transx::clone() const {
  return base_ptr(new mga_1dsm_transx(*this));
}

void mga_1dsm_transx::calc_objective(fitness_vector &f, const decision_vector &x, bool should_print, TransXSolution * solution) const {

  std::vector<double> T(get_n_legs());
  double alpha_sum = 0;
  for (int i = 0; i < T.size(); ++i) {
    T[i] = - log(x[T_idx(i)]);
    alpha_sum += T[i];
  }



  bool dsm = dsm_in_leg_i(0);

  kep_toolbox::array3D Vinf;
  if (dsm) {
    double theta = 2 * M_PI * x[vinf_index()];
    double phi = acos(2 * x[vinf_index() + 1] - 1) - M_PI / 2;

    double vinf = x[vinf_index() + 2];
    Vinf[0] = vinf * cos(phi) * cos(theta);
    Vinf[1] = vinf * cos(phi) * sin(theta);
    Vinf[2] = vinf * sin(phi);

    }

  for (int i = 0; i < T.size(); ++i) {
    T[i] = x[tof_index()] * T[i] / alpha_sum;
  }


  int n = get_seq().size();
  std::vector<kep_toolbox::epoch> t_P(n);
  std::vector<kep_toolbox::array3D> r_P(n);
  std::vector<kep_toolbox::array3D> v_P(n);
  std::vector<double> DV(n + 1, 0.0);

  for (int i = 0; i < n; ++i) {
    kep_toolbox::planet::planet_ptr planet = get_seq()[i];
    t_P[i] = kep_toolbox::epoch(x[t0_index()] + std::accumulate(T.begin(), T.begin() + i, 0.0));
    planet->eph(t_P[i], r_P[i], v_P[i]);
  }

  if (should_print) {
    transx_time_info(solution->mutable_times(), get_seq(), t_P);
  }

  kep_toolbox::array3D r, v;
  r = r_P[0];
  if (dsm) {
    if (m_add_vinf_dep) {
        DV[0] += burn_cost(get_seq()[0], Vinf, false, true);
    }
    if (should_print) {
        transx_escape(solution->mutable_escape(), get_seq()[0], v_P[0], r_P[0], Vinf, t_P[0].mjd());
    }

    kep_toolbox::array3D v0;
    kep_toolbox::sum(v0, v_P[0], Vinf);
    v = v0;
    kep_toolbox::propagate_lagrangian(r, v, (dsm ? x[DSM_idx(0)] : 0) * T[0] * ASTRO_DAY2SEC, get_common_mu());
  }

  double dt = (1 - (dsm ? x[DSM_idx(0)] : 0)) * T[0] * ASTRO_DAY2SEC;
  kep_toolbox::lambert_problem l(r, r_P[1], dt, get_common_mu());
  kep_toolbox::array3D v_end_l(l.get_v2()[0]);
  kep_toolbox::array3D v_beg_l(l.get_v1()[0]);

  if (!dsm) {
    kep_toolbox::diff(Vinf, v_beg_l, v_P[0]);
    v = Vinf;
    kep_toolbox::sum(v, v_P[0], Vinf);

    if (m_add_vinf_dep) {
        DV[0] += burn_cost(get_seq()[0], Vinf, false, true);
    }
    if (should_print) {
        transx_escape(solution->mutable_escape(), get_seq()[0], v_P[0], r_P[0], Vinf, t_P[0].mjd());
    }
  }

  kep_toolbox::array3D deltaV;
  kep_toolbox::diff(deltaV, v_beg_l, v);
  DV[0] += kep_toolbox::norm(deltaV);

  if (dsm && should_print) {
    transx_dsm(solution->add_dsms(), v, r, deltaV, v_beg_l, t_P[0].mjd() + T[0] - dt / ASTRO_DAY2SEC, 0);
  }

  for (int i = 1; i < n - 1; ++i) {
    dsm = dsm_in_leg_i(i);

    kep_toolbox::array3D v_rel_in, v_rel_out;
    if (dsm) {
        double radius = x[R_idx(i)] * get_seq()[i]->get_radius();
        double beta = x[Beta_idx(i)];
        kep_toolbox::array3D v_out;
        kep_toolbox::fb_prop(v_out, v_end_l, v_P[i], radius, beta, get_seq()[i]->get_mu_self());

        kep_toolbox::diff(v_rel_in, v_end_l, v_P[i]);
        kep_toolbox::diff(v_rel_out, v_out, v_P[i]);

        if (should_print) {
            transx_flyby(solution->add_flybyes(), get_seq()[i], v_P[i], r_P[i], v_rel_in, v_rel_out, t_P[i].mjd());
        }

        r = r_P[i]; v = v_out;

        kep_toolbox::propagate_lagrangian(r, v, (dsm ? x[DSM_idx(i)] : 0) * T[i] * ASTRO_DAY2SEC, get_common_mu());
    } else {
        r = r_P[i]; v = v_end_l;
    }

    dt = (1 - (dsm ? x[DSM_idx(i)] : 0)) * T[i] * ASTRO_DAY2SEC;
    kep_toolbox::lambert_problem l2(r, r_P[i + 1], dt, get_common_mu());
    v_beg_l = l2.get_v1()[0];
    v_end_l = l2.get_v2()[0];

    if (dsm) {
        kep_toolbox::diff(deltaV, v_beg_l, v);
        DV[i] = kep_toolbox::norm(deltaV);

        if (should_print) {
            transx_dsm(solution->add_dsms(), v, r, deltaV, v_beg_l, t_P[i].mjd() + T[i] - dt / ASTRO_DAY2SEC, 1);
        }
    } else {
        kep_toolbox::diff(v_rel_in, v, v_P[i]);
        kep_toolbox::diff(v_rel_out, v_beg_l, v_P[i]);

        kep_toolbox::planet::planet_ptr planet = get_seq()[i];
        kep_toolbox::fb_vel(DV[i], v_rel_in, v_rel_out, *planet);

        double ta  = acos(kep_toolbox::dot(v_rel_in, v_rel_out)/sqrt(kep_toolbox::dot(v_rel_in,v_rel_in))/sqrt(kep_toolbox::dot(v_rel_out,v_rel_out)));
        double alt = (planet->get_mu_self() / kep_toolbox::dot(v_rel_in,v_rel_in)*(1/sin(ta/2)-1) - planet->get_radius())/1000;
        if (alt > planet->get_safe_radius()) {
            f[0] = DBL_MAX;
            return;
        }

        if (should_print) {
            transx_flyby(solution->add_flybyes(), planet, v_P[i], r_P[i], v_rel_in, v_rel_out, t_P[i].mjd());
        }
    }

  }

  kep_toolbox::array3D Vexc_arr(v_end_l);
  kep_toolbox::diff(Vexc_arr, v_end_l, v_P[v_P.size() - 1]);
  if (m_add_vinf_arr) {
    DV[DV.size() - 1] += burn_cost(get_seq()[get_seq().size() - 1], Vexc_arr, true, get_circularize());
  }
  if (should_print) {
    transx_arrival(solution->mutable_arrival(), get_seq()[get_seq().size() - 1], Vexc_arr, t_P[t_P.size() - 1].mjd());
  }

  double sumDeltaV = std::accumulate(DV.begin(), DV.end(), 0.0);
  double sumT = std::accumulate(T.begin(), T.end(), 0.0);

  if (should_print) {
    solution->set_fuel_cost(sumDeltaV);
  }

  f[0] = sumDeltaV;
  if (get_f_dimension() == 2) {
    f[1] = sumT;
  }

}

std::string mga_1dsm_transx::get_name() const {
  return "MGA-1DSM";
}

}} // namespaces

BOOST_CLASS_EXPORT_IMPLEMENT(pagmo::problem::mga_1dsm_transx)
