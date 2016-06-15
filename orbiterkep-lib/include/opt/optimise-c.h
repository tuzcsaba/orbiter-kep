#include "orbiterkep-lib_Export.h"

#include <stdint.h>

struct _Orbiterkep__Parameters;

extern "C" orbiterkep_lib_EXPORT void __cdecl orbiterkep_optimize_multi(const uint8_t * param_buf, int param_l, double** pareto, int *N);

extern "C" orbiterkep_lib_EXPORT int __cdecl orbiterkep_optimize(const uint8_t * param_buf, int param_l, uint8_t * sol_buf);

extern "C" orbiterkep_lib_EXPORT int __cdecl param_hash(_Orbiterkep__Parameters const &param);
