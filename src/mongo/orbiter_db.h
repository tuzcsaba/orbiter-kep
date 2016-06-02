#ifndef ORBITERKEP_MONGO_DB_H_
#define ORBITERKEP_MONGO_DB_H_
#include "OrbiterKEPConfig.h"

#ifdef BUILD_MONGODB

#include <pagmo/pagmo.h>

#include "../param.h"
#include "../problems/transx_problem.h"

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

namespace orbiterkep {

class orbiterkep_db {
  public:
    orbiterkep_db();

    void store_solution(const parameters &params, const pagmo::problem::transx_solution &solution);

  protected:

  private:
      mongocxx::instance m_inst;
      mongocxx::client m_client;

};

} // namespaces

#endif

#endif
