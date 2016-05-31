#include <string>
#include <vector>
#include <numeric>
#include <cmath>
#include <iostream>
#include <boost/math/constants/constants.hpp>
#include <boost/array.hpp>

#include <keplerian_toolbox/core_functions/array3D_operations.h>
#include <keplerian_toolbox/core_functions/fb_vel.h>

#include "transx_problem.h"

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

void transx_problem::print_time_info(std::vector<kep_toolbox::planet::planet_ptr> planets, std::vector<kep_toolbox::epoch> times) const {
  std::vector<std::string> encounters;
  std::vector<std::pair<std::string, std::string> > transfers;
  for (int i = 0; i < planets.size() - 1; ++i) {
    kep_toolbox::planet::planet_ptr src = planets[i];
    kep_toolbox::planet::planet_ptr dst = planets[i + 1];
    std::cout << "Transfer time from " << src->get_name() << " to " << dst->get_name() << ":";
    std::cout << (times[i + 1].mjd() - times[i].mjd()) << std::setprecision(2) << " days" << std::endl;
  }

  for (int i = 0; i < planets.size(); ++i) {
    kep_toolbox::planet::planet_ptr x = planets[i];
    std::cout << "Date of " << x->get_name() << " encounter: ";
    std::cout << times[i] << std::endl;
  }

  std::cout << "Total mission duration: " << (times[times.size() - 1].mjd() - times[0].mjd()) << std::setprecision(2) << " days" << std::endl << std::endl << std::endl;
}

void transx_problem::print_escape(kep_toolbox::planet::planet_ptr ref, kep_toolbox::array3D V_ref, kep_toolbox::array3D R_ref, kep_toolbox::array3D deltaV, double eject_T) const {
  kep_toolbox::array3D dVTransX = velocity_to_transx(V_ref, R_ref, deltaV);
  double dv = kep_toolbox::norm(deltaV);

  std::cout << "Escape - " << ref->get_name() << std::endl;
  std::cout << "--------------------------------------" << std::endl;
  std::cout << "MJD:                   " << eject_T << std::setprecision(4) << std::endl;
  std::cout << "Prograde:              " << dVTransX[0] << std::setprecision(3) << std::endl;
  std::cout << "Outward:               " << dVTransX[1] << std::setprecision(3) << std::endl;
  std::cout << "Plane:                 " << dVTransX[2] << std::setprecision(3) << std::endl;
  std::cout << "Hyp. excess velocity:  " << dv << std::setprecision(3) <<  "m/s" << std::endl;
  double cost = burn_cost(ref, deltaV, false, true);
  std::cout << "Earth escape burn:     " << cost << std::setprecision(3) << std::endl;
  std::cout << std::endl << std::endl;
}

void transx_problem::print_dsm(kep_toolbox::array3D V_ref, kep_toolbox::array3D R_ref, kep_toolbox::array3D deltaV, kep_toolbox::array3D v, double dsm_T) const {
  kep_toolbox::array3D dVTransX = velocity_to_transx(V_ref, R_ref, deltaV);
  double dv = kep_toolbox::norm(deltaV);

  std::cout << "Deep Space Maneuver - " << std::endl;
  std::cout << "--------------------------------------" << std::endl;
  std::cout << "MJD:                   " << dsm_T << std::setprecision(4) << std::endl;
  std::cout << "Prograde:              " << dVTransX[0] << std::setprecision(3) << std::endl;
  std::cout << "Outward:               " << dVTransX[1] << std::setprecision(3) << std::endl;
  std::cout << "Plane:                 " << dVTransX[2] << std::setprecision(3) << std::endl;
  std::cout << "Hyp. excess velocity:  " << kep_toolbox::norm(v) << std::setprecision(3) <<  "m/s" << std::endl;
  std::cout << "DSM burn:              " << dv << std::setprecision(3) << std::endl;
  std::cout << std::endl << std::endl;
}

void transx_problem::print_flyby(kep_toolbox::planet::planet_ptr ref, kep_toolbox::array3D V_ref, kep_toolbox::array3D R_ref, kep_toolbox::array3D Vin, kep_toolbox::array3D Vout, double enc_T) const {
  kep_toolbox::array3D VoutTransX = velocity_to_transx(V_ref, R_ref, Vout);
  double vx = VoutTransX[0];
  double vy = VoutTransX[1];
  double vz = VoutTransX[2];

  std::cout << ref->get_name() << " encounter" << std::endl;
  std::cout << "--------------------------------------" << std::endl;
  std::cout << "MJD:                 " << enc_T << std::setprecision(4) << std::endl;
  std::cout << "Approach velocity:   " << sqrt(kep_toolbox::dot(Vin, Vin)) << std::setprecision(3) << std::endl;
  std::cout << "Departure velocity:  " << sqrt(kep_toolbox::dot(Vout,Vout)) << std::setprecision(3) << std::endl;  
  std::cout << "Outward angle:       " << 180*atan2(vy, vx)/M_PI << std::setprecision(3) << std::endl;
  std::cout << "Inclination:         " << 180*atan2(vz, sqrt(vx * vx + vy * vy))/M_PI << std::setprecision(3) << " deg" << std::endl;
  double ta  = acos(kep_toolbox::dot(Vin, Vout)/sqrt(kep_toolbox::dot(Vin,Vin))/sqrt(kep_toolbox::dot(Vout,Vout)));
  std::cout << "Turning angle:       " << ta*180/M_PI << std::setprecision(3) << " deg" << std::endl;
  double alt = (ref->get_mu_self() / kep_toolbox::dot(Vin,Vin)*(1/sin(ta/2)-1) - ref->get_radius())/1000;
  std::cout << "Periapsis altitude:  " << alt << std::setprecision(3) << " km" << std::endl;
  double enc_vel = 0.0;
  kep_toolbox::fb_vel(enc_vel, Vin, Vout, *ref);
  std::cout << "dV needed:           " << enc_vel << std::setprecision(3) << " m/s" << std::endl;
  std::cout << std::endl << std::endl;
}

void transx_problem::print_arrival(kep_toolbox::planet::planet_ptr ref, kep_toolbox::array3D V_exc, double enc_T) const {
  std::cout << ref->get_name() << " arrival" << std::endl;
  std::cout << "--------------------------------------" << std::endl;
  std::cout << "MJD:                        " << enc_T << std::setprecision(4) << "   " << std::endl;
  std::cout << "Hyp. excess velocity:       " << kep_toolbox::norm(V_exc) << std::setprecision(3) <<  "m/s" << std::endl;
  double cost = burn_cost(ref, V_exc, true, m_circularize);
  std::cout << "Orbit insertion burn " << cost << std::setprecision(3) << " m/s" << std::endl;
  std::cout << std::endl << std::endl;
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

void transx_problem::calc_objective(fitness_vector &f, const decision_vector &x, bool should_print) const {
  f[0] = 0;
  if (get_f_dimension() == 2) {
    f[1] = 0;
  }
}

void transx_problem::objfun_impl(fitness_vector &f, const decision_vector &x) const {
  calc_objective(f, x);
}

std::string transx_problem::pretty(const decision_vector &x, bool extended_output) const {
  fitness_vector f(get_f_dimension());
  calc_objective(f, x, true);
  return "";
}

std::string transx_problem::get_name() const {
  return "TransX base problem";
}

std::string transx_problem::human_readable_extra() const {
  return "TransX base problem - Do not use, subclass";
}

}} // namespaces

BOOST_CLASS_EXPORT_IMPLEMENT(pagmo::problem::transx_problem)
