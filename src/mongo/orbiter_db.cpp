#include "orbiter_db.h"

#ifdef BUILD_MONGODB

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/types.hpp>

#include <keplerian_toolbox/planet/jpl_low_precision.h>

using namespace bsoncxx;

namespace orbiterkep {

orbiterkep_db::orbiterkep_db():m_inst(mongocxx::instance{}),m_client(mongocxx::client{mongocxx::uri{}}) {
}

void orbiterkep_db::store_solution(const parameters &params, const pagmo::problem::transx_solution &solution) {

  using builder::stream::open_array;
  using builder::stream::close_array;
  using builder::stream::open_document;
  using builder::stream::close_document;
  using builder::stream::document;
  using builder::stream::finalize;


  auto collection = m_client["orbiter_kep"]["solutions"];

  int param_hash = boost::hash_value(params);

  mongocxx::options::find opts;
  opts.sort(document{} << "delta-v" << -1 << finalize);

  document query{};
  query << "param_hash" << param_hash;
  
  int count = collection.count(query << finalize);
  auto cursor = collection.find(query << finalize, opts);
  int i = count - 10;
  for (auto&& doc : cursor) {
    if (i <= 0) break;
    auto id = doc["_id"].get_value();
    auto deltaV = doc["delta-v"].get_value().get_double();
    if (deltaV < solution.fuel_cost) {
      return;
    }
    collection.delete_one(document{} << "_id" << id << finalize);
    --i;
  }

  builder::stream::document document{};

  document << "param_hash" << param_hash;
  
  auto planetsArr = document << "planets" << open_array; 
  for (int i = 0; i < params.planets.size(); ++i) {
    auto planet = params.planets[i];
    planetsArr << planet->get_name();
  }
  planetsArr << close_array;

  auto launch = solution.times.times[0].mjd();
  document << "problem" << solution.problem;
  document << "launch_mjd" << launch;
  document << "eject_altitude" << params.dep_altitude;
  document << "target_altitude" << params.arr_altitude;
  document << "delta-v" << solution.fuel_cost;

  document << "transx_plan" << solution.string();

  auto decisionVec = document << "decision_vector" << open_array;
  for (int i = 0; i < solution.x.size(); ++i) {
    decisionVec << solution.x[i];
  }
  decisionVec << close_array;

  collection.insert_one(document.view());
};

} // namespaces

#endif
