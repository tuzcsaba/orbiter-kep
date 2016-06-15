#include "opt/optimise-c.h"

#include "model/param.h"
#include "opt/optimise.h"

#include "proto/parameters.pb-c.h"
#include "proto/solution.pb-c.h"

struct membuf : std::streambuf
{
    membuf(char* begin, char* end) {
        this->setg(begin, begin, end);
    }
};

int __cdecl param_hash(Orbiterkep__Parameters const &param) {
    return boost::hash_value(param);
}

void __cdecl calc_solution(const uint8_t * param_buf, int param_l, orbiterkep::TransXSolution &sol_cpp, bool multi, std::vector<std::pair<double, double> > &pareto) {
    membuf sbuf((char *)param_buf, (char *)param_buf + param_l);
  std::istream in(&sbuf);
  orbiterkep::Parameters param_cpp;
  param_cpp.ParseFromIstream(&in);

  param_cpp.set_multi_objective(multi);
  int gen = param_cpp.pagmo().n_gen() / 10;
  param_cpp.mutable_pagmo()->set_n_gen(gen);

  std::cout << param_cpp.planets(0) << std::endl;
  std::cout << param_cpp.dep_altitude() << std::endl;

  orbiterkep::optimiser::optimize(param_cpp, &sol_cpp, pareto);
}

void __cdecl orbiterkep_optimize_multi(const uint8_t * param_buf, int param_l, double** pareto_buf, int *maxN) {
    orbiterkep::TransXSolution sol_cpp;
    std::vector<std::pair<double, double> > pareto;
    calc_solution(param_buf, param_l, sol_cpp, true, pareto);

    int N = 0;
    for (int i = 0; i < pareto.size(); ++i) {
        if (N >= *maxN) {
            break;
        }
        pareto_buf[i][0] = pareto[i].first;
        pareto_buf[i][1] = pareto[i].second;
        ++N;
    }
    *maxN = N;
    return;
}

int __cdecl orbiterkep_optimize(const uint8_t * param_buf, int param_l, uint8_t * sol_buf) {

  orbiterkep::TransXSolution sol_cpp;
  std::vector<std::pair<double, double> > pareto;
  calc_solution(param_buf, param_l, sol_cpp, false, pareto);

  std::string result_raw;
  sol_cpp.SerializeToString(&result_raw);
  int len = result_raw.length();
  memcpy(sol_buf, result_raw.c_str(), len);
  return len;
}

