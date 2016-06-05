#include "proto/solution.pb.h"
#include "proto/parameters.pb.h"

namespace orbiterkep {

std::ostream& operator<<(std::ostream& os, const TransXSolution &sol); 
std::ostream& operator<<(std::ostream& os, const TransXTimes &times);
std::ostream& operator<<(std::ostream& os, const TransXEscape &times);
std::ostream& operator<<(std::ostream& os, const TransXDSM &dsm);
std::ostream& operator<<(std::ostream& os, const TransXFlyBy &flyBy);
std::ostream& operator<<(std::ostream& os, const TransXArrival &times);



}


