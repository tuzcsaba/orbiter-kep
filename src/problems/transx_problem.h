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

#include "proto/solution.pb.h"

namespace pagmo { namespace problem {

std::ostream& operator<<(std::ostream& os, const TransXSolution &sol); 
std::ostream& operator<<(std::ostream& os, const TransXTimes &times);
std::ostream& operator<<(std::ostream& os, const TransXEscape &times);
std::ostream& operator<<(std::ostream& os, const TransXDSM &dsm);
std::ostream& operator<<(std::ostream& os, const TransXFlyBy &flyBy);
std::ostream& operator<<(std::ostream& os, const TransXArrival &times);

class __PAGMO_VISIBLE transx_problem : public base
{
  public:
    transx_problem(const std::vector<kep_toolbox::planet::planet_ptr> = construct_default_sequence(),
        const double dep_altitude = 300, const double arr_altitude = 300, const bool circularize = false,
        const int dim = 1, const int obj_dim = 1);
    transx_problem(const transx_problem&);
    base_ptr clone() const;



    virtual std::string get_name() const;
    void fill_solution(TransXSolution * solution, const decision_vector &x, bool extended_output = false) const;


    std::vector<kep_toolbox::planet::planet_ptr> get_seq() const { return m_seq; }
    int    get_n_legs() const       { return m_n_legs; }
    bool   get_circularize() const  { return m_circularize; }
    double get_common_mu() const    { return m_common_mu; }
    double get_dep_altitude() const { return m_dep_altitude; }
    double get_arr_altitude() const { return m_arr_altitude; }

    virtual void calc_objective(fitness_vector &f, const decision_vector &x, bool should_print = false, TransXSolution * solution = 0) const;

  protected:
    void transx_time_info(TransXTimes *times, std::vector<kep_toolbox::planet::planet_ptr> planets, std::vector<kep_toolbox::epoch> time_list) const;
    void transx_escape(TransXEscape *escape, kep_toolbox::planet::planet_ptr ref, kep_toolbox::array3D V_ref, 
        kep_toolbox::array3D R_ref, kep_toolbox::array3D deltaV, double eject_T) const;
    void transx_dsm(TransXDSM * dsm, kep_toolbox::array3D V_ref, kep_toolbox::array3D R_ref, kep_toolbox::array3D deltaV, kep_toolbox::array3D v, double dsm_T) const;
    void transx_flyby(TransXFlyBy *flyBy, kep_toolbox::planet::planet_ptr ref, kep_toolbox::array3D V_ref, kep_toolbox::array3D R_ref, kep_toolbox::array3D Vin, kep_toolbox::array3D Vout, double enc_T) const;
    void transx_arrival(TransXArrival *arrival, kep_toolbox::planet::planet_ptr ref, kep_toolbox::array3D V_exc, double enc_T) const;

    double burn_cost(kep_toolbox::planet::planet_ptr ref, const kep_toolbox::array3D &exc, bool arr, bool circ) const;
    kep_toolbox::array3D velocity_to_transx(kep_toolbox::array3D v_ref, kep_toolbox::array3D v_rad, kep_toolbox::array3D v) const;


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
