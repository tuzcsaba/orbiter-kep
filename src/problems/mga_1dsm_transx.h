#ifndef PAGMO_PROBLEM_MGA_1DSM_TRANSX_H
#define PAGMO_PROBLEM_MGA_1DSM_TRANSX_H

#include <keplerian_toolbox/planet/jpl_low_precision.h>
#include <keplerian_toolbox/epoch.h>
#include <pagmo/config.h>
#include <pagmo/serialization.h>
#include <pagmo/types.h>
#include <pagmo/problem/base.h>
#include "transx_problem.h"


namespace pagmo { namespace problem { 

class __PAGMO_VISIBLE mga_1dsm_transx : public transx_problem {
public:
    mga_1dsm_transx(const std::vector<kep_toolbox::planet::planet_ptr> seq = construct_default_sequence(),
                    const double dep_altitude = 300, const double arr_altitude = 300, const bool circularize = false,
                    const kep_toolbox::epoch t0_l = kep_toolbox::epoch(0), const kep_toolbox::epoch t0_u = kep_toolbox::epoch(1000),
                    const double tof_l = 1.0, const double tof_u = 5.0,
                    const double vinf_l = 0.1, const double vinf_u = 8.0,
                    const bool add_vinf_dep = false, const bool add_vinf_arr = true,
                    const bool multi_objective = false, const bool dsm_allowed = false);
    mga_1dsm_transx(const mga_1dsm_transx&);
    base_ptr clone() const;

    bool get_multi_obj() const { return m_multi_obj; }
    bool get_add_vinf_dep() const { return m_add_vinf_dep; }
    bool get_add_vinf_arr() const { return m_add_vinf_arr; }
    bool get_dsm_allowed() const { return m_dsm_allowed; }

    virtual std::string get_name() const;

    virtual void calc_objective(fitness_vector &f, const decision_vector &x, bool should_print = false, TransXSolution * solution = 0) const;

private:

    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive &ar, const unsigned int ver) {
      transx_problem::serialize(ar, ver);
      ar & m_multi_obj;
      ar & m_add_vinf_dep;
      ar & m_add_vinf_arr;
      ar & m_dsm_allowed;
    }

    bool m_multi_obj;
    bool m_add_vinf_dep;
    bool m_add_vinf_arr;
    bool m_dsm_allowed;
};

}} // namespaces;w

BOOST_CLASS_EXPORT_KEY(pagmo::problem::mga_1dsm_transx)
#endif
