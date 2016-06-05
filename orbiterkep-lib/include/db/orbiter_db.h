#ifndef ORBITERKEP_MONGO_DB_H_
#define ORBITERKEP_MONGO_DB_H_
#include "OrbiterKEPConfig.h"

#ifdef BUILD_MONGODB

#include "model/param.h"
#include "problems/transx_problem.h"

#include <pagmo/pagmo.h>

struct _mongoc_client_t;
typedef _mongoc_client_t mongoc_client_t;

namespace orbiterkep {

class orbiterkep_db {
  public:
    orbiterkep_db();
    ~orbiterkep_db();

    void store_solution(const Parameters &params, const pagmo::problem::TransXSolution &solution, const std::string &problem);

    pagmo::decision_vector get_stored_solution(const Parameters &params, const std::string &problem);

  protected:

  private:
      mongoc_client_t *m_client;

};

} // namespaces

#endif

#endif
