#include "model/param.h"

namespace orbiterkep {

    CommandLine::CommandLine() {
        m_args["planets"] = new TCLAP::ValueArg<std::string>("", "planets",  "encountered planetary bodies", false,
                                                             "earth,venus,mercury", "earth,venus,mercury");
        m_args["launch"] = new TCLAP::ValueArg<std::string>("", "launch",   "the bounds for the time of flight", false,
                                                            "20000101T000000,20140101T000000",
                                                            "20000101T000000,20140101T000000");

        m_args["tof-min"] = new TCLAP::ValueArg<double>("", "tof-min",  "minimum time of flight", false, 0.1, "0.1");
        m_args["tof-max"] = new TCLAP::ValueArg<double>("", "tof-max",  "maximum time of flight", false, 7, "7");
        m_args["vinf-min"] = new TCLAP::ValueArg<double>("", "vinf-min", "minimum allowed initial Hyperbolic Excess Velocity in km/s (check Launcher performance)", false, 0.1, "0.1");
        m_args["vinf-max"] = new TCLAP::ValueArg<double>("", "vinf-max", "maximum allowed initial Hyperbolic Excess Velocity in km/s (check Launcher performance)", false, 10.0, "10");

        m_args["n-trials"] = new TCLAP::ValueArg<int>("", "n-mga",      "the number of independent optimisations", false, 1, "0");
        m_args["problem"] = new TCLAP::ValueArg<std::string>("", "problem", "the type of problem", false, "MGA", "MGA");

        m_args["dep-altitude"] = new TCLAP::ValueArg<double>("", "dep-altitude", "the ejection altitude in kms", false, 300.0, "300.0");
        m_args["arr-altitude"] = new TCLAP::ValueArg<double>("", "arr-altitude", "the target altitude at the target body in kms", false, 300.0, "300.0");

        m_args["multi-obj"] = new TCLAP::SwitchArg ("", "multi-obj", "if true, then we optimise for Delta-V and Time of Flight", false);
        m_args["capture-only"] = new TCLAP::SwitchArg("", "capture-only", "if on, the fuel calculations at arrival assume circular target orbit", false);

        m_args["omit-dep-vinf"] = new TCLAP::SwitchArg("", "omit-dep-vinf", "if on, the fuel calculations omit the final capture burn", false);
        m_args["omit-arr-vinf"] = new TCLAP::SwitchArg("", "omit-arr-vinf", "if on, the fuel calculations won't include the initial Vinf", false);
        m_args["use-db"] = new TCLAP::SwitchArg("", "use-db", "if on, the fuel calculations won't include the initial Vinf", false);
        m_args["spice"] = new TCLAP::SwitchArg("", "spice", "use JPL SPICE toolkit for getting ephimerides", false);
        m_args["max-delta-v"] = new TCLAP::ValueArg<double>("", "max-delta-v", "Delta-V budget", false, 20000, "m/s");


        m_args["algos-single-obj"] = new TCLAP::ValueArg<std::string>("","algos-single-obj", "the algorithms to use in single-object optimisation", false, "jde", "jde");
        m_args["algos-multi-obj"] = new TCLAP::ValueArg<std::string>("","algos-multi-obj", "the algorithms to use in single-object optimisation", false, "nsga2", "nsga2");

        m_args["pagmo-gen"] = new TCLAP::ValueArg<int>("", "pagmo-gen", "the number of generations to run each trial for", false, 10000, "10000");
        m_args["pagmo-islands"] = new TCLAP::ValueArg<int>("", "pagmo-islands", "the number of (parallel) islands to run. Set to number of CPUs", false, 8, "8");
        m_args["pagmo-population"] = new TCLAP::ValueArg<int>("", "pagmo-population", "the number of speciments in each island", false, 60, "60");
        m_args["pagmo-mf"] = new TCLAP::ValueArg<int>("", "pagmo-mf", "the migration frequency in the algorithm", false, 150, "150");
        m_args["pagmo-mr"] = new TCLAP::ValueArg<double>("", "pagmo-mr", "the migration rate to use in PAGMO", false, 0.15, "0.15");
    }

    CommandLine::~CommandLine() {

    }

    void prepare_args() {

    }

    void cleanup_args() {

    }

    void CommandLine::configure_command(TCLAP::CmdLine &cmd) {

        for (auto arg : m_args) {
            cmd.add( *(arg.second) );
        }

    }

    bool CommandLine::switchArgValue(const std::string &key) {
        auto arg = (TCLAP::SwitchArg *)m_args[key];
        if (arg == 0) {
          std::cerr << "No argument with key '" << key << "' is configured";
        }
        return arg->getValue();
    }

    template<typename T> T CommandLine::argValue(const std::string &key) {
        auto arg = (TCLAP::ValueArg<T> *)m_args[key];
        if (arg == 0) {
          std::cerr << "No argument with key '" << key << "' is configured";
        }
        return arg->getValue();
    }

    std::vector<kep_toolbox::planet::planet_ptr> CommandLine::kep_toolbox_planets(const Parameters &param) {
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

    bool CommandLine::parse_parameters(Parameters * param, int argc, char ** argv) {

        try {
            TCLAP::CmdLine cmd("Calculate Optimal MGA and MGA-1DSM trajectories", ' ', "0.1");

            configure_command(cmd);

            if (argc == 1 || argv == NULL) return false;
            cmd.parse(argc, argv);

            param->set_use_spice(switchArgValue("spice"));

            if (param->use_spice()) {
                CommandLine::load_spice_kernels();
            }

            std::vector<std::string> planets;
            std::string planetsStr = argValue<std::string>("planets");
            boost::split(planets, planetsStr, boost::is_any_of(","));
            for (int i = 0; i < planets.size(); ++i) {
                param->add_planets(planets[i]);
            }

            param->set_dep_altitude( argValue<double>("dep-altitude") );
            param->set_arr_altitude( argValue<double>("arr-altitude") );
            param->set_circularize( switchArgValue("capture-only") );

            std::vector<std::string> t0s;
            std::string t0sStr = argValue<std::string>("launch");
            boost::split(t0s, t0sStr, boost::is_any_of(","));
            param->mutable_t0()->set_lb(kep_toolbox::epoch_from_iso_string(t0s[0]).mjd());
            param->mutable_t0()->set_ub(kep_toolbox::epoch_from_iso_string(t0s[1]).mjd());

            param->mutable_tof()->set_lb( argValue<double>("tof-min") );
            param->mutable_tof()->set_ub( argValue<double>("tof-max") );

            param->mutable_vinf()->set_lb( argValue<double>("vinf-min") );
            param->mutable_vinf()->set_ub( argValue<double>("vinf-max") );

            param->set_add_arr_vinf( !switchArgValue("omit-arr-vinf") );
            param->set_add_dep_vinf( !switchArgValue("omit-dep-vinf") );

            param->set_multi_objective( switchArgValue("multi-obj") );
            param->set_max_deltav( argValue<double>("max-delta-v") );
            param->set_problem(argValue<std::string>("problem"));
            param->set_n_trials(argValue<int>("n-trials"));


            auto pagmo = param->mutable_pagmo();
            pagmo->set_n_gen( argValue<int>("pagmo-gen") );
            pagmo->set_n_isl( argValue<int>("pagmo-islands") );
            pagmo->set_population( argValue<int>("pagmo-population") );
            pagmo->set_mf( argValue<int>("pagmo-mf") );
            pagmo->set_mr( argValue<double>("pagmo-mr") );

            std::vector<std::string> algos_single;
            std::string algosSingleStr = argValue<std::string>("algos-single-obj");
            boost::split(algos_single, algosSingleStr, boost::is_any_of(","));
            for (int i = 0; i < algos_single.size(); ++i) {
                param->add_single_objective_algos( algos_single[i] );
            }

            std::vector<std::string> algos_multi;
            std::string algosMultiStr = argValue<std::string>("algos-multi-obj");
            boost::split(algos_multi, algosMultiStr, boost::is_any_of(","));
            for (int i = 0; i < algos_multi.size(); ++i) {
                param->add_multi_objective_algos( algos_multi[i] );
            }

            param->set_use_db( switchArgValue("use-db"));

            return true;
        } catch (TCLAP::ArgException &e) {
            std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
            return false;
        }
    };

    void CommandLine::load_spice_kernels() {
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
