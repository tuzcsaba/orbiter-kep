#ifndef ORBITERKEP_PARAMS_H_
#define ORBITERKEP_PARAMS_H_

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <keplerian_toolbox/planet/jpl_low_precision.h>
#include <keplerian_toolbox/planet/spice.h>

#include <tclap/CmdLine.h>

namespace orbiterkep {

struct parameters {
  std::vector<kep_toolbox::planet::planet_ptr> planets;
  std::vector<std::string> single_object_algos;
  std::vector<std::string> multi_object_algos;

  kep_toolbox::epoch t0[2];
  double tof[2];
  double vinf[2];

  int n_mga;
  int n_mga_1dsm;

  int n_gen;

  double dep_altitude;
  double arr_altitude;

  double max_deltaV;

  bool add_dep_vinf;
  bool add_arr_vinf;

  bool spice;

  bool circularize;

  bool multi_obj;
};

void load_spice_kernels();

parameters parse_parameters(int argc, char ** argv);

}

namespace boost {

  static inline std::size_t hash_value(orbiterkep::parameters const &p) {
    std::size_t seed = 0;

    for (int i = 0; i < p.planets.size(); ++i) {
      boost::hash_combine(seed, p.planets[i]->get_name());
    }

    boost::hash_combine(seed, p.t0[0].mjd());
    boost::hash_combine(seed, p.t0[1].mjd());

    boost::hash_combine(seed, p.tof[0]);
    boost::hash_combine(seed, p.tof[1]);

    boost::hash_combine(seed, p.vinf[0]);
    boost::hash_combine(seed, p.vinf[1]);

    boost::hash_combine(seed, p.dep_altitude);
    boost::hash_combine(seed, p.arr_altitude);

    boost::hash_combine(seed, p.spice);
    boost::hash_combine(seed, p.circularize);

    boost::hash_combine(seed, p.add_dep_vinf);
    boost::hash_combine(seed, p.add_arr_vinf);

    return seed;
  }



}

#endif
