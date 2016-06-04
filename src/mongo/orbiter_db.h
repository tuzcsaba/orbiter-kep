#ifndef ORBITERKEP_MONGO_DB_H_
#define ORBITERKEP_MONGO_DB_H_
#include "OrbiterKEPConfig.h"

#ifdef BUILD_MONGODB

#include <pagmo/pagmo.h>

#include "../param.h"
#include "../problems/transx_problem.h"

#include <mongoc.h>

namespace orbiterkep {

class orbiterkep_db {
  public:
    orbiterkep_db();
    ~orbiterkep_db();

    void store_solution(const parameters &params, const pagmo::problem::TransXSolution &solution, const std::string &problem);

    pagmo::decision_vector get_stored_solution(const parameters &params, const std::string &problem);

  protected:

  private:
      mongoc_client_t *m_client;

};

} // namespaces

#endif

#endif
