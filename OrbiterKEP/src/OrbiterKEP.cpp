#include "OrbiterKEPConfig.h"

#include <string>
#include <iostream>

#include "model/param.h"
#include "opt/optimise.h"

#include "proto/ext.h"

int main(int argc, char **argv) {

    orbiterkep::Parameters param;
    
    if (!orbiterkep::parse_parameters(&param, argc, argv)) {
      return -1;
    }

    std::cout << std::fixed;
    std::cerr << std::fixed;

    orbiterkep::TransXSolution solution;

    orbiterkep::optimize(param, &solution);

    std::cout << solution << std::endl;

  return 0;
}
