#include "proto/solution.pb.h"
#include "proto/parameters.pb.h"
#include "orbiterkep-lib_Export.h"

namespace orbiterkep {

orbiterkep_lib_EXPORT std::ostream& operator<<(std::ostream& os, const TransXSolution &sol);
orbiterkep_lib_EXPORT std::ostream& operator<<(std::ostream& os, const TransXTimes &times);
orbiterkep_lib_EXPORT std::ostream& operator<<(std::ostream& os, const TransXEscape &times);
orbiterkep_lib_EXPORT std::ostream& operator<<(std::ostream& os, const TransXDSM &dsm);
orbiterkep_lib_EXPORT std::ostream& operator<<(std::ostream& os, const TransXFlyBy &flyBy);
orbiterkep_lib_EXPORT std::ostream& operator<<(std::ostream& os, const TransXArrival &times);



}


