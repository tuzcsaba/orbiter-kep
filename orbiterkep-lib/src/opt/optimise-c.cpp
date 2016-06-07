#include "opt/optimise-c.h"

#include "opt/optimise.h"

_Orbiterkep__TransXSolution * orbiterkep_optimize(const _Orbiterkep__Parameters &params) {
  int len = orbiterkep__parameters__get_packed_size(&params);
  uint8_t *buf;
  buf = (uint8_t *)malloc(len);
  orbiterkep__parameters__pack(&params, buf);

  std::string raw(reinterpret_cast<char const*>(buf), len);
  orbiterkep::Parameters param_cpp;
  param_cpp.ParseFromString(raw);
  free(buf);

  orbiterkep::TransXSolution sol_cpp;
  orbiterkep::optimiser::optimize(param_cpp, &sol_cpp);

  std::string result_raw;
  sol_cpp.SerializeToString(&result_raw);
  const uint8_t *c = (uint8_t *)result_raw.c_str();
  len = result_raw.length();

  return orbiterkep__trans_xsolution__unpack(NULL, len, c);
}
