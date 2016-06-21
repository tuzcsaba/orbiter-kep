#include "problems/transx_problem.h"

#include <cmath>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include <boost/math/constants/constants.hpp>
#include <boost/array.hpp>

#include <keplerian_toolbox/core_functions/array3D_operations.h>
#include <keplerian_toolbox/core_functions/fb_vel.h>

namespace pagmo { namespace problem {

transx_problem::transx_problem(const std::vector<kep_toolbox::planet::planet_ptr> seq,
    const double dep_altitude, const double arr_altitude, const bool circularize,
    const int dim, const int obj_dim) : base(dim, 0, obj_dim, 0, 0, 0.0), m_n_legs(seq.size() - 1), m_arr_altitude(arr_altitude), m_dep_altitude(dep_altitude), m_circularize(circularize)
{
  std::vector<double> mus(seq.size());
  for (std::vector<kep_toolbox::planet::planet_ptr>::size_type i = 0; i < seq.size(); ++i) {
    mus[i] = seq[i]->get_mu_central_body();
  }
  if ((unsigned int)std::count(mus.begin(), mus.end(), mus[0]) != mus.size()) {
    pagmo_throw(value_error, "The planets do not all have the same mu_central_body");
  }

  for (std::vector<kep_toolbox::planet::planet_ptr>::size_type i = 0; i < seq.size(); ++i) {
    m_seq.push_back(seq[i]->clone());
  }

  m_common_mu = seq[0]->get_mu_central_body();
  m_max_deltaV = 20000.0;
}

transx_problem::transx_problem(const transx_problem &p) : base(p.get_dimension(), 0, p.get_f_dimension(), 0, 0, 0, 0), m_seq(), m_common_mu(p.m_common_mu), m_n_legs(p.m_n_legs), m_circularize(p.m_circularize), m_arr_altitude(p.m_arr_altitude), m_dep_altitude(p.m_dep_altitude) {
  for (std::vector<kep_toolbox::planet::planet_ptr>::size_type i = 0; i < p.m_seq.size(); ++i) {
    m_seq.push_back(p.m_seq[i]->clone());
  }

  set_bounds(p.get_lb(),p.get_ub());
}

base_ptr transx_problem::clone() const {
  return base_ptr(new transx_problem(*this));
}
void transx_problem::transx_time_info(TransXTimes *times, std::vector<kep_toolbox::planet::planet_ptr> planets, std::vector<kep_toolbox::epoch> time_list) const {
  for (int i = 0; i < planets.size(); ++i) {
    times->add_planets(planets[i]->get_name());
    times->add_times(time_list[i].mjd());
  }
}

void transx_problem::transx_escape(TransXEscape *escape, kep_toolbox::planet::planet_ptr ref, kep_toolbox::array3D V_ref, kep_toolbox::array3D R_ref, kep_toolbox::array3D deltaV, double eject_T) const {

  kep_toolbox::array3D dVTransX = velocity_to_transx(V_ref, R_ref, deltaV);
  double dv = kep_toolbox::norm(deltaV);

  escape->set_planet(ref->get_name());
  escape->set_mjd(eject_T);

  escape->set_prograde(dVTransX[0]);
  escape->set_outward(dVTransX[1]);
  escape->set_plane(dVTransX[2]);

  escape->set_vinf(dv);

  escape->set_burn(burn_cost(ref, deltaV, false, true));
}

void transx_problem::transx_dsm(TransXDSM * dsm, kep_toolbox::array3D V_ref, kep_toolbox::array3D R_ref, kep_toolbox::array3D deltaV, kep_toolbox::array3D v, double dsm_T, int leg) const {
  kep_toolbox::array3D dVTransX = velocity_to_transx(V_ref, R_ref, deltaV);
  double dv = kep_toolbox::norm(deltaV);

  dsm->set_mjd(dsm_T);

  dsm->set_prograde(dVTransX[0]);
  dsm->set_outward(dVTransX[1]);
  dsm->set_plane(dVTransX[2]);

  dsm->set_vinf(kep_toolbox::norm(v));

  dsm->set_burn(dv);

  dsm->set_leg(leg);
}

void transx_problem::transx_flyby(TransXFlyBy *flyby, kep_toolbox::planet::planet_ptr ref, kep_toolbox::array3D V_ref, kep_toolbox::array3D R_ref, kep_toolbox::array3D Vin, kep_toolbox::array3D Vout, double enc_T) const {
  kep_toolbox::array3D VoutTransX = velocity_to_transx(V_ref, R_ref, Vout);
  double vx = VoutTransX[0];
  double vy = VoutTransX[1];
  double vz = VoutTransX[2];

  flyby->set_planet( ref->get_name() );
  flyby->set_mjd( enc_T );

  flyby->set_approach_vel( sqrt(kep_toolbox::dot(Vin, Vin)) );
  flyby->set_departure_vel( sqrt(kep_toolbox::dot(Vout, Vout)) );

  flyby->set_outward_angle( 180 * atan2(vy, vx) / M_PI );
  flyby->set_inclination( 180 * atan2(vz, sqrt(vx * vx + vy * vy)) / M_PI );

  double ta  = acos(kep_toolbox::dot(Vin, Vout)/sqrt(kep_toolbox::dot(Vin,Vin))/sqrt(kep_toolbox::dot(Vout,Vout)));
  flyby->set_turning_angle( ta * 180.0 / M_PI );

  double alt = (ref->get_mu_self() / kep_toolbox::dot(Vin,Vin)*(1/sin(ta/2)-1) - ref->get_radius())/1000;
  flyby->set_periapsis_altitude( alt );

  double enc_vel = 0.0;
  kep_toolbox::fb_vel(enc_vel, Vin, Vout, *ref);
  flyby->set_burn(enc_vel);
}

void transx_problem::transx_arrival(TransXArrival *arrival, kep_toolbox::planet::planet_ptr ref, kep_toolbox::array3D V_exc, double enc_T) const {

  arrival->set_planet(ref->get_name());
  arrival->set_mjd(enc_T);
  arrival->set_vinf(kep_toolbox::norm(V_exc));
  arrival->set_burn(burn_cost(ref, V_exc, true, m_circularize));
}

kep_toolbox::array3D transx_problem::velocity_to_transx(kep_toolbox::array3D v_ref, kep_toolbox::array3D v_rad, kep_toolbox::array3D v) const {

  kep_toolbox::array3D fward;
  kep_toolbox::array3D plane;
  kep_toolbox::array3D oward;

  kep_toolbox::vers(fward, v_ref);
  kep_toolbox::cross(plane, v_ref, v_rad);
  kep_toolbox::vers(plane, plane);
  kep_toolbox::cross(oward, plane, fward);

  kep_toolbox::array3D res;
  res[0] = kep_toolbox::dot(fward, v);
  res[1] = kep_toolbox::dot(oward, v);
  res[2] = kep_toolbox::dot(plane, v);

  return res;
}

double transx_problem::burn_cost(kep_toolbox::planet::planet_ptr ref, const kep_toolbox::array3D &exc, bool arr, bool circ) const {
  double mu = ref->get_mu_self();
  double r = ref->get_radius() + (arr ? m_arr_altitude : m_dep_altitude) * 1000;
  double norm_sqr = kep_toolbox::dot(exc, exc);
  return sqrt(norm_sqr + 2 * mu / r) - sqrt((circ ? 1 : 2) * mu / r);
}

void transx_problem::calc_objective(fitness_vector &f, const decision_vector &x, bool should_print, TransXSolution * solution) const {
  f[0] = 0;
  if (get_f_dimension() == 2) {
    f[1] = 0;
  }
}

void transx_problem::objfun_impl(fitness_vector &f, const decision_vector &x) const {
  calc_objective(f, x);
}

void transx_problem::fill_solution(TransXSolution * solution, const decision_vector &x, bool extended_output) const {
  fitness_vector f(get_f_dimension());
  calc_objective(f, x, true, solution);
  for (int i =0; i < x.size(); ++i) {
    solution->add_x(x[i]);
  }
  solution->set_problem(get_name());
}

std::string transx_problem::get_name() const {
  return "TransX base problem";
}

std::string transx_problem::human_readable_extra() const {
  return "TransX base problem - Do not use, subclass";
}

}} // namespaces

BOOST_CLASS_EXPORT_IMPLEMENT(pagmo::problem::transx_problem)
