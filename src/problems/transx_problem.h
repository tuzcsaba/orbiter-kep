#ifndef PAGMO_PROBLEM_TRANSX_H
#define PAGMO_PROBLEM_TRANSX_H

#include <string>
#include <boost/array.hpp>
#include <keplerian_toolbox/planet/jpl_low_precision.h>
#include <keplerian_toolbox/epoch.h>
#include <pagmo/config.h>
#include <pagmo/serialization.h>
#include <pagmo/types.h>
#include <pagmo/problem/base.h>

namespace pagmo { namespace problem {

struct transx_escape {
  std::string planet;
  double mjd;

  double prograde;
  double outward;
  double plane;

  double v_inf;

  double burn;

  std::string string() const;
};

struct transx_dsm {
  double mjd;

  double prograde;
  double outward;
  double plane;

  double v_inf;

  double burn;

  std::string string() const;
};

struct transx_flyby {
  std::string planet;

  double mjd;

  double approach_vel;
  double departure_vel;

  double outward_angle;
  double inclination;
  double turning_angle;

  double periapsis_altitude;

  double burn;

  std::string string() const;
};

struct transx_arrival {
  std::string planet;

  double mjd;

  double v_inf;

  double burn;

  std::string string() const;
};

struct transx_times {
  std::vector<std::string> planets;

  std::vector<kep_toolbox::epoch> times;

  std::string string() const;
};

struct transx_solution {
  transx_times times;

  transx_escape escape;
  std::vector<transx_dsm> dsms;
  std::vector<transx_flyby> flybyes;
  transx_arrival arrival;

  double fuel_cost;

  std::string string() const;
};

class __PAGMO_VISIBLE transx_problem : public base
{
  public:
    transx_problem(const std::vector<kep_toolbox::planet::planet_ptr> = construct_default_sequence(),
        const double dep_altitude = 300, const double arr_altitude = 300, const bool circularize = false,
        const int dim = 1, const int obj_dim = 1);
    transx_problem(const transx_problem&);
    base_ptr clone() const;



    std::string get_name() const;
    std::string pretty(const decision_vector &x, bool extended_output = false) const;


    std::vector<kep_toolbox::planet::planet_ptr> get_seq() const { return m_seq; }
    int    get_n_legs() const       { return m_n_legs; }
    bool   get_circularize() const  { return m_circularize; }
    double get_common_mu() const    { return m_common_mu; }
    double get_dep_altitude() const { return m_dep_altitude; }
    double get_arr_altitude() const { return m_arr_altitude; }

  protected:
    transx_times transx_time_info(std::vector<kep_toolbox::planet::planet_ptr> planets, std::vector<kep_toolbox::epoch> times) const;
    transx_escape transx_escape(kep_toolbox::planet::planet_ptr ref, kep_toolbox::array3D V_ref, 
        kep_toolbox::array3D R_ref, kep_toolbox::array3D deltaV, double eject_T) const;
    transx_dsm transx_dsm(kep_toolbox::array3D V_ref, kep_toolbox::array3D R_ref, kep_toolbox::array3D deltaV, kep_toolbox::array3D v, double dsm_T) const;
    transx_flyby transx_flyby(kep_toolbox::planet::planet_ptr ref, kep_toolbox::array3D V_ref, kep_toolbox::array3D R_ref, kep_toolbox::array3D Vin, kep_toolbox::array3D Vout, double enc_T) const;
    transx_arrival transx_arrival(kep_toolbox::planet::planet_ptr ref, kep_toolbox::array3D V_exc, double enc_T) const;

    double burn_cost(kep_toolbox::planet::planet_ptr ref, const kep_toolbox::array3D &exc, bool arr, bool circ) const;
    kep_toolbox::array3D velocity_to_transx(kep_toolbox::array3D v_ref, kep_toolbox::array3D v_rad, kep_toolbox::array3D v) const;

    virtual void calc_objective(fitness_vector &f, const decision_vector &x, bool should_print = false) const;


    void objfun_impl(fitness_vector &, const decision_vector &) const;
    std::string human_readable_extra() const;
  protected:
    static const std::vector<kep_toolbox::planet::planet_ptr> construct_default_sequence() {
      std::vector<kep_toolbox::planet::planet_ptr> retval;
      retval.push_back(kep_toolbox::planet::jpl_lp("earth").clone());
      retval.push_back(kep_toolbox::planet::jpl_lp("venus").clone());
      retval.push_back(kep_toolbox::planet::jpl_lp("earth").clone());
      return retval;
    }
    static const std::vector<boost::array<double, 2> > construct_default_tof() {
      std::vector<boost::array<double, 2> > retval;
      boost::array<double,2> dumb = {{ 50,900 }};  
			retval.push_back(dumb);
			retval.push_back(dumb);
			return retval;
    }
  protected:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive &ar, const unsigned int) {
      ar & boost::serialization::base_object<base>(*this);
      ar & m_seq;
      ar & m_dep_altitude;
      ar & m_arr_altitude;
      ar & m_circularize;
      ar & m_max_deltaV;
    }

  private: 
    std::vector<kep_toolbox::planet::planet_ptr> m_seq;
    const size_t m_n_legs;
    double m_common_mu;
    double m_arr_altitude;
    double m_dep_altitude;
    bool m_circularize;
    double m_max_deltaV;
};

}} // namespaces

BOOST_CLASS_EXPORT_KEY(pagmo::problem::transx_problem)
#endif
