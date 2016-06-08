#ifndef ORBITERKEP_PARAMS_H_
#define ORBITERKEP_PARAMS_H_

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <keplerian_toolbox/planet/jpl_low_precision.h>
#include <keplerian_toolbox/planet/spice.h>

#include <tclap/CmdLine.h>

#include "proto/ext.h"

namespace orbiterkep {

void load_spice_kernels();

orbiterkep_lib_EXPORT bool parse_parameters(Parameters * param, int argc, char ** argv);

std::vector<kep_toolbox::planet::planet_ptr> kep_toolbox_planets(const Parameters &param);

}
namespace boost {

  static inline std::size_t hash_value(orbiterkep::Parameters const &p) {
    std::size_t seed = 0;

    for (int i = 0; i < p.planets_size(); ++i) {
      boost::hash_combine(seed, p.planets(i));
    }

    boost::hash_combine(seed, p.t0().lb());
    boost::hash_combine(seed, p.t0().ub());

    boost::hash_combine(seed, p.tof().lb());
    boost::hash_combine(seed, p.tof().ub());

    boost::hash_combine(seed, p.vinf().lb());
    boost::hash_combine(seed, p.vinf().ub());

    boost::hash_combine(seed, p.dep_altitude());
    boost::hash_combine(seed, p.arr_altitude());

    boost::hash_combine(seed, p.use_spice());
    boost::hash_combine(seed, p.circularize());

    boost::hash_combine(seed, p.add_dep_vinf());
    boost::hash_combine(seed, p.add_arr_vinf());

    return seed;
  }



}

#endif
