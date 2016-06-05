#include "model/param.h"

namespace orbiterkep {
    
    TCLAP::ValueArg<std::string> planetsArg("", "planets",  "encountered planetary bodies", false,
                                            "earth,venus,mercury", "earth,venus,mercury");
    
    TCLAP::ValueArg<std::string> launchArg ("", "launch",   "the bounds for the time of flight", false,
                                            "20000101T000000,20140101T000000",
                                            "20000101T000000,20140101T000000");
    
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
    
    
    void configure_command(TCLAP::CmdLine &cmd) {
        
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
        
    }

    std::vector<kep_toolbox::planet::planet_ptr> kep_toolbox_planets(const Parameters &param) {
        std::vector<kep_toolbox::planet::planet_ptr> seq;
        for (int i = 0; i < param.planets_size(); ++i) {
            kep_toolbox::planet::jpl_lp lp_planet(param.planets(i));
            if (param.use_spice()) {
                seq.push_back(kep_toolbox::planet::spice(param.planets(i), "SUN", "ECLIPJ2000", "NONE",
                                                         lp_planet.get_mu_central_body(),
                                                         lp_planet.get_mu_self(),
                                                         lp_planet.get_radius(),
                                                         lp_planet.get_safe_radius()).clone());
            } else {
                seq.push_back(lp_planet.clone());
            }
        }
        return seq;
    }
    
    bool parse_parameters(Parameters * param, int argc, char ** argv) {
        
      try {
        TCLAP::CmdLine cmd("Calculate Optimal MGA and MGA-1DSM trajectories", ' ', "0.1");
        
        configure_command(cmd);
        
        if (argc == 1 || argv == NULL) return false;
        cmd.parse(argc, argv);
        
        param->set_use_spice(spiceArg.getValue());

        if (param->use_spice()) {
            load_spice_kernels();
        }
        
        std::vector<std::string> planets;
        boost::split(planets, planetsArg.getValue(), boost::is_any_of(","));
        for (int i = 0; i < planets.size(); ++i) {
          param->add_planets(planets[i]);
        }
        
        param->set_dep_altitude( depAltArg.getValue() );
        param->set_arr_altitude( arrAltArg.getValue() );
        param->set_circularize( captureOnlyArg.getValue() );
        
        std::vector<std::string> t0s;
        boost::split(t0s, launchArg.getValue(), boost::is_any_of(","));
        param->mutable_t0()->set_min(kep_toolbox::epoch_from_iso_string(t0s[0]).mjd());
        param->mutable_t0()->set_max(kep_toolbox::epoch_from_iso_string(t0s[1]).mjd());
        
        param->mutable_tof()->set_min( tofMinArg.getValue() );
        param->mutable_tof()->set_max( tofMaxArg.getValue() );
        
        param->mutable_vinf()->set_min( 0.1 );
        param->mutable_vinf()->set_max( vinfMaxArg.getValue() );
        
        param->set_add_arr_vinf( !omitArrVInfArg.getValue() );
        param->set_add_dep_vinf( !omitDepVInfArg.getValue() );
        
        param->set_multi_objective( multiObjArg.getValue() );
        param->set_max_deltav( maxDeltaVArg.getValue() );
        if (nMGAArg.getValue() > 0) {
          param->set_problem( "MGA" );
          param->set_n_trials( nMGAArg.getValue() );
        } else if (nMGA1DSMArg.getValue() > 0) {
          param->set_problem( "MGA-1DSM" );
          param->set_n_trials( nMGA1DSMArg.getValue() );
        }

        param->set_n_gen( optGenArg.getValue() );
        
        std::vector<std::string> algos_single;
        boost::split(algos_single, algoSingleArg.getValue(), boost::is_any_of(","));
        for (int i = 0; i < algos_single.size(); ++i) {
          param->add_single_objective_algos( algos_single[i] );
        }
        
        std::vector<std::string> algos_multi;
        boost::split(algos_multi, algoMultiArg.getValue(), boost::is_any_of(","));
        for (int i = 0; i < algos_multi.size(); ++i) {
          param->add_multi_objective_algos( algos_multi[i] );
        }
        
        param->set_use_db( useDBArg.getValue() );

        return true;
      } catch (TCLAP::ArgException &e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        return false;
      }
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
