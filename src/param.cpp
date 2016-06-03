#include "param.h"

namespace orbiterkep {

parameters parse_parameters(int argc, char ** argv) {

    TCLAP::CmdLine cmd("Calculate Optimal MGA and MGA-1DSM trajectories", ' ', "0.1");

    TCLAP::ValueArg<std::string> planetsArg("", "planets",  "encountered planetary bodies", false, "earth,venus,mercury", "earth,venus,mercury");
    TCLAP::ValueArg<std::string> launchArg ("", "launch",   "the bounds for the time of flight", false, "20000101T000000,20140101T000000", "20000101T000000,20140101T000000");
    TCLAP::ValueArg<double>      tofMinArg ("", "tof-min",  "minimum time of flight", false, 0.5, "0.5");
    TCLAP::ValueArg<double>      tofMaxArg ("", "tof-max",  "maximum time of flight", false, 0.5, "0.5");
    TCLAP::ValueArg<double>      vinfMaxArg   ("", "vinf-max", "maximum allowed initial Hyperbolic Excess Velocity in km/s (check Launcher performance)", false, 10, "10");

    TCLAP::ValueArg<int>         nMGAArg   ("", "n-mga",      "the number of independent MGA optimisations", false, 0, "0");
    TCLAP::ValueArg<int>         nMGA1DSMArg  ("", "n-mga-1dsm", "the number of independent MGA-1DSM optimisations", false, 0, "0");

    TCLAP::ValueArg<double>      depAltArg ("", "dep-altitude", "the ejection altitude in kms", false, 300.0, "300.0");
    TCLAP::ValueArg<double>      arrAltArg ("", "arr-altitude", "the target altitude at the target body in kms", false, 300.0, "300.0");

    TCLAP::SwitchArg             multiObjArg("", "multi-obj", "if true, then we optimise for Delta-V and Time of Flight", false);
    TCLAP::SwitchArg             captureOnlyArg("", "capture-only", "if on, the fuel calculations at arrival assume circular target orbit", false);
    
    TCLAP::SwitchArg             omitDepVInfArg("", "omit-dep-vinf", "if on, the fuel calculations omit the final capture burn", false);
    TCLAP::SwitchArg             omitArrVInfArg("", "omit-arr-vinf", "if on, the fuel calculations won't include the initial Vinf", false);
    TCLAP::SwitchArg             useDBArg("", "use-db", "if on, the fuel calculations won't include the initial Vinf", false);
    TCLAP::SwitchArg             spiceArg("", "spice", "use JPL SPICE toolkit for getting ephimerides", false);
    TCLAP::ValueArg<double>      maxDeltaVArg("", "max-delta-v", "Delta-V budget", false, 20000, "m/s");
    
    TCLAP::ValueArg<int>      optGenArg("", "opt-gen", "the number of generations to run each trial for", false, 10000, "10000");

    TCLAP::ValueArg<std::string> algoSingleArg("","algos-single-obj", "the algorithms to use in single-object optimisation", false, "jde", "jde");
    TCLAP::ValueArg<std::string> algoMultiArg("","algos-multi-obj", "the algorithms to use in single-object optimisation", false, "nsga2", "nsga2");

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

    cmd.add( algoSingleArg );
    cmd.add( algoMultiArg );

    cmd.add( useDBArg );

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

    std::vector<std::string> algos_single;
    boost::split(algos_single, algoSingleArg.getValue(), boost::is_any_of(","));
    param.single_object_algos = algos_single;

    std::vector<std::string> algos_multi;
    boost::split(algos_multi, algoMultiArg.getValue(), boost::is_any_of(","));
    param.multi_object_algos = algos_multi;

    param.use_db = useDBArg.getValue();

    return param;
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


} // namespaces
