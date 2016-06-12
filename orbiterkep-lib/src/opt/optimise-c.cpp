#include "opt/optimise-c.h"

#include "opt/optimise.h"

#include "proto/parameters.pb-c.h"
#include "proto/solution.pb-c.h"

struct membuf : std::streambuf
{
    membuf(char* begin, char* end) {
        this->setg(begin, begin, end);
    }
};

int __cdecl orbiterkep_optimize(const uint8_t * param_buf, int param_l, uint8_t * sol_buf) {

  membuf sbuf((char *)param_buf, (char *)param_buf + param_l);
  std::istream in(&sbuf);
  orbiterkep::Parameters param_cpp;
  param_cpp.ParseFromIstream(&in);

  std::cout << param_cpp.planets(0) << std::endl;
  std::cout << param_cpp.dep_altitude() << std::endl;

  orbiterkep::TransXSolution sol_cpp;
  orbiterkep::optimiser::optimize(param_cpp, &sol_cpp);

  std::string result_raw;
  sol_cpp.SerializeToString(&result_raw);
  int len = result_raw.length();
  memcpy(sol_buf, result_raw.c_str(), len);
  return len;
}

