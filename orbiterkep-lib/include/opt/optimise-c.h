#include "proto/parameters.pb-c.h"
#include "proto/solution.pb-c.h"

#include "orbiterkep-lib_Export.h"

extern "C" orbiterkep_lib_EXPORT _Orbiterkep__TransXSolution * orbiterkep_optimize(const _Orbiterkep__Parameters &params);
