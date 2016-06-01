#include <string>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <keplerian_toolbox/planet/jpl_low_precision.h>
#include <keplerian_toolbox/planet/spice.h>
#include <keplerian_toolbox/util/spice_utils.h>
#include <pagmo/pagmo.h>
#include <tclap/CmdLine.h>

#include "src/problems/mga_transx.h"
#include "src/problems/mga_1dsm_transx.h"
#include "src/optimise.h"

namespace orbiterkep {

struct parameters {
  std::vector<kep_toolbox::planet::planet_ptr> planets;

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

void load_spice_kernels() {
    if (boost::filesystem::exists("./spice")) {
      std::cout << "Loading SPICE kernels:" << std::endl;
      boost::filesystem::path directory("./spice");
      boost::filesystem::directory_iterator iter(directory), end;
      for (; iter != end; ++iter) {
        boost::filesystem::path kernel_path = iter->path();
        std::string spice_kernel_filename = kernel_path.string();
        std::cout << "- " << spice_kernel_filename << std::endl;
        kep_toolbox::util::load_spice_kernel(spice_kernel_filename);
      }
    } else {
      std::cout << "No 'spice' folder found, skipping loading SPICE kernels";
    }


};

parameters parse_parameters(int argc, char ** argv) {

    TCLAP::CmdLine cmd("Calculate Optimal MGA and MGA-1DSM trajectories", ' ', "0.1");

    TCLAP::ValueArg<std::string> planetsArg("", "planets",  "encountered planetary bodies", false, "earth,venus,mercury", "p_1,p_2,...,p_n");
    TCLAP::ValueArg<std::string> launchArg ("", "launch",   "the bounds for the time of flight", false, "20000101T000000,20140101T000000", "min_tof,max_tof");
    TCLAP::ValueArg<double>      tofMinArg ("", "tof-min",  "minimum time of flight", false, 0.5, "min_tof");
    TCLAP::ValueArg<double>      tofMaxArg ("", "tof-max",  "minimum time of flight", false, 0.5, "max_tof");
    TCLAP::ValueArg<double>      vinfMaxArg   ("", "vinf-max", "maximum allowed initial Hyperbolic Excess Velocity in km/s (check Launcher performance)", false, 5, "max_vinf");

    TCLAP::ValueArg<int>         nMGAArg   ("", "n-mga",      "the number of independent MGA optimisations", false, 0, "n");
    TCLAP::ValueArg<int>         nMGA1DSMArg  ("", "n-mga-1dsm", "the number of independent MGA-1DSM optimisations", false, 0, "n");

    TCLAP::ValueArg<double>      depAltArg ("", "dep-altitude", "the ejection altitude in kms", false, 300.0, "f");
    TCLAP::ValueArg<double>      arrAltArg ("", "arr-altitude", "the target altitude at the target body in kms", false, 300.0, "f");

    TCLAP::SwitchArg             multiObjArg("", "multi-obj", "if true, then we optimise for Delta-V and Time of Flight", false);
    TCLAP::SwitchArg             captureOnlyArg("", "capture-only", "if on, the fuel calculations at arrival assume circular target orbit", false);
    
    TCLAP::SwitchArg             omitDepVInfArg("", "omit-dep-vinf", "if on, the fuel calculations omit the final capture burn", false);
    TCLAP::SwitchArg             omitArrVInfArg("", "omit-arr-vinf", "if on, the fuel calculations won't include the initial Vinf", false);
    TCLAP::SwitchArg             spiceArg("", "spice", "use JPL SPICE toolkit for getting ephimerides", false);
    TCLAP::ValueArg<double>      maxDeltaVArg("", "max-delta-v", "Delta-V budget", false, 20000, "m/s");
    
    TCLAP::ValueArg<int>      optGenArg("", "opt-gen", "the number of generations to run each trial for", false, 10000, "n");

    cmd.add( planetsArg );
    cmd.add( launchArg );
    cmd.add( tofMinArg ); cmd.add( tofMaxArg );
    cmd.add( vinfMaxArg );
    cmd.add( nMGAArg ); cmd.add( nMGA1DSMArg );
    cmd.add( depAltArg ); cmd.add( arrAltArg );
    cmd.add( multiObjArg );
    cmd.add( captureOnlyArg );
    cmd.add( maxDeltaVArg );
    cmd.add( omitDepVInfArg ); cmd.add( omitArrVInfArg );
    cmd.add( optGenArg );
    cmd.add( spiceArg );

    cmd.parse(argc, argv);

    parameters param;

    param.spice = spiceArg.getValue();
    if (param.spice) {
      load_spice_kernels();
    }

    std::vector<std::string> planets;
    boost::split(planets, planetsArg.getValue(), boost::is_any_of(","));
    std::vector<kep_toolbox::planet::planet_ptr> seq;
    for (int i = 0; i < planets.size(); ++i) {
      kep_toolbox::planet::jpl_lp lp_planet(planets[i]);
      if (param.spice) {
        seq.push_back(kep_toolbox::planet::spice(planets[i], "SUN", "ECLIPJ2000", "NONE",
              lp_planet.get_mu_central_body(),
              lp_planet.get_mu_self(),
              lp_planet.get_radius(),
              lp_planet.get_safe_radius()).clone());
      } else {
        seq.push_back(lp_planet.clone());
      }
    }
    param.planets = seq;

    param.dep_altitude = depAltArg.getValue();
    param.arr_altitude = arrAltArg.getValue();
    param.circularize  = captureOnlyArg.getValue();

    std::vector<std::string> t0s;
    boost::split(t0s, launchArg.getValue(), boost::is_any_of(","));
    param.t0[0] = kep_toolbox::epoch_from_iso_string(t0s[0]);
    param.t0[1] = kep_toolbox::epoch_from_iso_string(t0s[1]);

    param.tof[0] = tofMinArg.getValue();
    param.tof[1] = tofMaxArg.getValue();

    param.vinf[0] = 0.5;
    param.vinf[1] = vinfMaxArg.getValue();

    param.add_arr_vinf = !omitArrVInfArg.getValue();
    param.add_dep_vinf = !omitDepVInfArg.getValue();

    param.multi_obj = multiObjArg.getValue();
    param.max_deltaV = maxDeltaVArg.getValue();
    param.n_mga = nMGAArg.getValue();
    param.n_mga_1dsm = nMGA1DSMArg.getValue();

    param.n_gen = optGenArg.getValue();

    return param;
};

} // namespaces
int main(int argc, char **argv) {
  try {

    orbiterkep::parameters param = orbiterkep::parse_parameters(argc, argv);

    std::cout << std::fixed;
    std::cerr << std::fixed;

    if (param.n_mga > 0) {
      std::cout << "Executing MGA trials" << std::endl;

      pagmo::problem::mga_transx mga(param.planets, 
          param.dep_altitude, param.arr_altitude, param.circularize, 
          param.t0[0], param.t0[1], param.tof[0], param.tof[1], 
          param.vinf[0], param.vinf[1],
          param.add_dep_vinf, param.add_arr_vinf,
          false);

      pagmo::problem::mga_transx mga_multi(param.planets,
          param.dep_altitude, param.arr_altitude, param.circularize, 
          param.t0[0], param.t0[1], param.tof[0], param.tof[1], 
          param.vinf[0], param.vinf[1],
          param.add_dep_vinf, param.add_arr_vinf,
           true);

      std::cout << "- Running single-objective optimisation";
      orbiterkep::optimiser op(mga, param.n_mga, param.n_gen, 100, 1);
      pagmo::decision_vector sol_mga = op.run_once(0, false, param.max_deltaV);
      std::cout << " Done" << std::endl;

      if (param.multi_obj) {
        std::cout << "- Running multi-objective optimisation";
        orbiterkep::optimiser op_multi(mga_multi, param.n_mga, param.n_gen, 100, 1);
        op_multi.run_once(&sol_mga, true, param.max_deltaV);
        std::cout << "Done" << std::endl;
      }

      mga.pretty(sol_mga);

    }
    if (param.n_mga_1dsm > 0) {
      std::cout << "Executing MGA-1DSM trials" << std::endl;

      pagmo::problem::mga_1dsm_transx mga_1dsm(param.planets,
          param.dep_altitude, param.arr_altitude, param.circularize, 
          param.t0[0], param.t0[1], param.tof[0], param.tof[1], 
          param.vinf[0], param.vinf[1],
          param.add_dep_vinf, param.add_arr_vinf,
          false);


      pagmo::problem::mga_1dsm_transx mga_1dsm_multi(param.planets,
          param.dep_altitude, param.arr_altitude, param.circularize, 
          param.t0[0], param.t0[1], param.tof[0], param.tof[1], 
          param.vinf[0], param.vinf[1],
          param.add_dep_vinf, param.add_arr_vinf,
          true);

      std::cout << "- Running single-objective optimisation" << std::flush;
      orbiterkep::optimiser op2(mga_1dsm, param.n_mga_1dsm, param.n_gen, 100, 1);
      pagmo::decision_vector sol_mga_1dsm = op2.run_once(0, false, param.max_deltaV);
      std::cout << "Done" << std::endl;

      if (param.multi_obj) {
        std::cout << "- Running multi-objective optimisation" << std::flush;
        orbiterkep::optimiser op2_multi(mga_1dsm_multi, param.n_mga_1dsm, param.n_gen, 100, 1);
        op2_multi.run_once(&sol_mga_1dsm, true, param.max_deltaV);
        std::cout << "Done" << std::endl;
      }

      mga_1dsm.pretty(sol_mga_1dsm);
    }
  } catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
  }
  return 0;
}
