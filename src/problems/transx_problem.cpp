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

std::string transx_times::string() const {
  std::stringstream ss;
  ss << std::fixed;

  std::vector<std::string> encounters;
  std::vector<std::pair<std::string, std::string> > transfers;
  for (int i = 0; i < planets.size() - 1; ++i) {
    ss << "Transfer time from " << planets[i] << " to " << planets[i + 1] << ":";
    ss << (times[i + 1].mjd() - times[i].mjd()) << std::setprecision(2) << " days" << std::endl;
  }

  for (int i = 0; i < planets.size(); ++i) {
    ss << "Date of " << planets[i] << " encounter: ";
    ss << times[i] << std::endl;
  }

  ss << "Total mission duration: " << (times[times.size() - 1].mjd() - times[0].mjd()) << std::setprecision(2) << " days" << std::endl << std::endl << std::endl;

  return ss.str();
}

transx_times transx_problem::transx_time_info(std::vector<kep_toolbox::planet::planet_ptr> planets, std::vector<kep_toolbox::epoch> time_list) const {
  std::vector<std::string> p;
  for (int i = 0; i < planets.size(); ++i) {
    p.push_back(planets[i]->get_name());
  }
  transx_times times;
  times.planets = p;
  times.times = time_list;
  return times;
}

std::string transx_escape::string() const {
  std::stringstream ss;

  ss << std::fixed;
  ss << "Escape - " << planet << std::endl;
  ss << "--------------------------------------" << std::endl;
  ss << "MJD:                     " << mjd << std::setprecision(4) << std::endl;
  ss << "Prograde:                " << prograde << std::setprecision(3) << std::endl;
  ss << "Outward:                 " << outward << std::setprecision(3) << std::endl;
  ss << "Plane:                   " << plane << std::setprecision(3) << std::endl;
  ss << "Hyp. excess velocity:    " << v_inf << std::setprecision(3) <<  " m/s" << std::endl;
  ss << "Earth escape burn:       " << burn << std::setprecision(3) << std::endl;
  ss << std::endl << std::endl;

  return ss.str();
};

transx_escape transx_problem::transx_escape(kep_toolbox::planet::planet_ptr ref, kep_toolbox::array3D V_ref, kep_toolbox::array3D R_ref, kep_toolbox::array3D deltaV, double eject_T) const {
  struct transx_escape escape;
  kep_toolbox::array3D dVTransX = velocity_to_transx(V_ref, R_ref, deltaV);
  double dv = kep_toolbox::norm(deltaV);

  escape.planet = ref->get_name();
  escape.mjd = eject_T;
  
  escape.prograde = dVTransX[0];
  escape.outward = dVTransX[1];
  escape.plane = dVTransX[2];

  escape.v_inf = dv;

  escape.burn = burn_cost(ref, deltaV, false, true);

  return escape;
}

std::string transx_dsm::string() const {
  std::stringstream ss;

  ss << std::fixed;
  ss << "Deep Space Maneuver - " << std::endl;
  ss << "--------------------------------------" << std::endl;
  ss << "MJD:                     " << mjd << std::setprecision(4) << std::endl;
  ss << "Prograde:                " << prograde << std::setprecision(3) << std::endl;
  ss << "Outward:                 " << outward << std::setprecision(3) << std::endl;
  ss << "Plane:                   " << plane << std::setprecision(3) << std::endl;
  ss << "Hyp. excess velocity:    " << v_inf << std::setprecision(3) <<  "m/s" << std::endl;
  ss << "DSM burn:                " << burn << std::setprecision(3) << std::endl;
  
  ss << std::endl << std::endl;

  return ss.str();
}

transx_dsm transx_problem::transx_dsm(kep_toolbox::array3D V_ref, kep_toolbox::array3D R_ref, kep_toolbox::array3D deltaV, kep_toolbox::array3D v, double dsm_T) const {
  kep_toolbox::array3D dVTransX = velocity_to_transx(V_ref, R_ref, deltaV);
  double dv = kep_toolbox::norm(deltaV);

  struct transx_dsm dsm;
  dsm.mjd = dsm_T;

  dsm.prograde = dVTransX[0];
  dsm.outward =  dVTransX[1];
  dsm.plane   =  dVTransX[2];

  dsm.v_inf = kep_toolbox::norm(v);

  dsm.burn = dv;

  return dsm;
}

std::string transx_flyby::string() const {
  std::stringstream ss; 

  ss << std::fixed;
  ss << planet << " encounter" << std::endl;
  ss << "--------------------------------------" << std::endl;
  ss << "MJD:                   " << mjd << std::setprecision(4) << std::endl;
  ss << "Approach velocity:     " << approach_vel << std::setprecision(3) << std::endl;
  ss << "Departure velocity:    " << departure_vel << std::setprecision(3) << std::endl;  

  ss << "Outward angle:         " << outward_angle << std::endl;
  ss << "Inclination:           " << inclination << std::setprecision(3) << " deg" << std::endl;
  ss << "Turning angle:         " << turning_angle << std::setprecision(3) << " deg" << std::endl;
  ss << "Periapsis altitude:    " << periapsis_altitude << std::setprecision(3) << " km" << std::endl;
  ss << "dV needed:             " << burn << std::setprecision(3) << " m/s" << std::endl;
  ss << std::endl << std::endl;

  return ss.str();
}

transx_flyby transx_problem::transx_flyby(kep_toolbox::planet::planet_ptr ref, kep_toolbox::array3D V_ref, kep_toolbox::array3D R_ref, kep_toolbox::array3D Vin, kep_toolbox::array3D Vout, double enc_T) const {
  kep_toolbox::array3D VoutTransX = velocity_to_transx(V_ref, R_ref, Vout);
  double vx = VoutTransX[0];
  double vy = VoutTransX[1];
  double vz = VoutTransX[2];

  struct transx_flyby flyby;
  flyby.planet = ref->get_name();
  flyby.mjd = enc_T;

  flyby.approach_vel = sqrt(kep_toolbox::dot(Vin, Vin));
  flyby.departure_vel = sqrt(kep_toolbox::dot(Vout, Vout));
  
  flyby.outward_angle = 180 * atan2(vy, vx) / M_PI;
  flyby.inclination = 180 * atan2(vz, sqrt(vx * vx + vy * vy)) / M_PI;

  double ta  = acos(kep_toolbox::dot(Vin, Vout)/sqrt(kep_toolbox::dot(Vin,Vin))/sqrt(kep_toolbox::dot(Vout,Vout)));
  flyby.turning_angle = ta * 180.0 / M_PI;

  double alt = (ref->get_mu_self() / kep_toolbox::dot(Vin,Vin)*(1/sin(ta/2)-1) - ref->get_radius())/1000;
  flyby.periapsis_altitude = alt;

  double enc_vel = 0.0;
  kep_toolbox::fb_vel(enc_vel, Vin, Vout, *ref);
  flyby.burn = enc_vel;
  return flyby;
}

std::string transx_arrival::string() const {
  std::stringstream ss;

  ss << std::fixed;
  ss << planet << " arrival" << std::endl;
  ss << "--------------------------------------" << std::endl;
  ss << "MJD:                     " << mjd << std::setprecision(4) << "   " << std::endl;
  ss << "Hyp. excess velocity:    " << v_inf << std::setprecision(3) <<  "m/s" << std::endl;
  ss << "Orbit insertion burn:    " << burn << std::setprecision(3) << " m/s" << std::endl;
  ss << std::endl << std::endl;

  return ss.str();
}

transx_arrival transx_problem::transx_arrival(kep_toolbox::planet::planet_ptr ref, kep_toolbox::array3D V_exc, double enc_T) const {
  struct transx_arrival arrival;

  arrival.planet = ref->get_name();
  arrival.mjd = enc_T;
  arrival.v_inf = kep_toolbox::norm(V_exc);
  arrival.burn = burn_cost(ref, V_exc, true, m_circularize);

  return arrival;
}

std::string transx_solution::string() const
{
  std::stringstream ss;

  ss << times.string();

  ss << escape.string();

  int i = 0; 
  int j = 0;
  while (i < dsms.size() || j < flybyes.size()) {
    if (i < dsms.size()) {
      ss << dsms[i++].string();
    }
    if (j < flybyes.size()) {
      ss << flybyes[j++].string();
    }
  }

  ss << arrival.string();

  ss << "Total delta-V:           " << fuel_cost << std::setprecision(3) << " m/s" << std::endl;

  return ss.str();
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
