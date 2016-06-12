#ifndef ORBITERKEP_LIB_PROTO_EXT_C_H
#define ORBITERKEP_LIB_PROTO_EXT_C_H

#include "orbiterkep-lib_Export.h"

#ifdef __cplusplus
extern "C" {
#endif
orbiterkep_lib_EXPORT int __cdecl sprintf_transx_solution(char * buf, const Orbiterkep__TransXSolution * solution);
orbiterkep_lib_EXPORT int __cdecl sprintf_transx_times(char * buf, const Orbiterkep__TransXTimes * times);
orbiterkep_lib_EXPORT int __cdecl sprintf_transx_escape(char * buf, const Orbiterkep__TransXEscape * escape);
orbiterkep_lib_EXPORT int __cdecl sprintf_transx_dsm(char * buf, const Orbiterkep__TransXDSM * dsm);
orbiterkep_lib_EXPORT int __cdecl sprintf_transx_flyby(char * buf, const Orbiterkep__TransXFlyBy * flyby);
orbiterkep_lib_EXPORT int __cdecl sprintf_transx_arrival(char * buf, const Orbiterkep__TransXArrival * arrival);
#ifdef __cplusplus
}
#endif

#endif
