#include <string>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <keplerian_toolbox/planet/jpl_low_precision.h>
#include <pagmo/pagmo.h>
#include <tclap/CmdLine.h>

#include "src/problems/mga_transx.h"
#include "src/problems/mga_1dsm_transx.h"
#include "src/optimise.h"

int main(int argc, char **argv) {
  try {
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

    cmd.add(omitDepVInfArg); cmd.add(omitArrVInfArg);

    cmd.add(optGenArg);

    cmd.parse(argc, argv);
    std::vector<std::string> planets;
    boost::split(planets, planetsArg.getValue(), boost::is_any_of(","));
    std::vector<kep_toolbox::planet::planet_ptr> seq;
    for (int i = 0; i < planets.size(); ++i) {
      seq.push_back(kep_toolbox::planet::jpl_lp(planets[i]).clone());
    }

    double dep_altitude = depAltArg.getValue();
    double arr_altitude = arrAltArg.getValue();
    bool circularize = !captureOnlyArg.getValue();

    std::vector<std::string> t0s;
    boost::split(t0s, launchArg.getValue(), boost::is_any_of(","));
    kep_toolbox::epoch t0_l = kep_toolbox::epoch_from_iso_string(t0s[0]);
    kep_toolbox::epoch t0_u = kep_toolbox::epoch_from_iso_string(t0s[1]);

    double tof_l = tofMinArg.getValue();
    double tof_u = tofMaxArg.getValue();

    double vinf_l = 0.5;
    double vinf_u = vinfMaxArg.getValue();

    const bool add_arr_vinf = !omitArrVInfArg.getValue();
    const bool add_dep_vinf = !omitDepVInfArg.getValue();

    bool multi_obj = multiObjArg.getValue();

    double maxDeltaV = maxDeltaVArg.getValue();
    
    int n_trial_mga = nMGAArg.getValue();
    int n_trial_mga_1dsm = nMGA1DSMArg.getValue();

    int n_gen = optGenArg.getValue();

    if (n_trial_mga > 0) {

      pagmo::problem::mga_transx mga(seq, 
          dep_altitude, arr_altitude, circularize, 
          t0_l, t0_u, tof_l, tof_u, 
          vinf_l, vinf_u,
          add_dep_vinf, add_arr_vinf,
          false);

      pagmo::problem::mga_transx mga_multi(seq,
          arr_altitude, arr_altitude, circularize,
           t0_l, t0_u, tof_l, tof_u,
           vinf_l, vinf_u, 
           add_dep_vinf, add_arr_vinf,
           true);

      orbiterkep::optimiser op(mga, n_trial_mga, n_gen, 100, 1);
      pagmo::decision_vector sol_mga = op.run_once(0, maxDeltaV);
      mga.pretty(sol_mga);

      if (multi_obj) {
        orbiterkep::optimiser op_multi(mga_multi, n_trial_mga, n_gen, 100, 1);
        op_multi.run_once(&sol_mga, maxDeltaV);
      }
    }
    if (n_trial_mga_1dsm > 0) {

      pagmo::problem::mga_1dsm_transx mga_1dsm(seq,
          dep_altitude, arr_altitude, circularize,
          t0_l, t0_u, tof_l, tof_u,
          vinf_l, vinf_u,
          add_dep_vinf, add_arr_vinf,
          false);

      std::cout << mga_1dsm.get_dimension() << std::endl;

      pagmo::problem::mga_1dsm_transx mga_1dsm_multi(seq,
          dep_altitude, arr_altitude, circularize,
          t0_l, t0_u, tof_l, tof_u,
          vinf_l, vinf_u,
          add_dep_vinf, add_arr_vinf,
          true);

      orbiterkep::optimiser op2(mga_1dsm, n_trial_mga_1dsm, n_gen, 100, 1);
      pagmo::decision_vector sol_mga_1dsm = op2.run_once(0, maxDeltaV);
      mga_1dsm.pretty(sol_mga_1dsm);

      if (multi_obj) {
        orbiterkep::optimiser op2_multi(mga_1dsm_multi, n_trial_mga, n_gen, 100, 1);
        op2_multi.run_once(&sol_mga_1dsm, maxDeltaV);
      }
    }
  } catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
  }
  return 0;
}
