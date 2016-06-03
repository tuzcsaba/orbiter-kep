#include "orbiter_db.h"

#ifdef BUILD_MONGODB

#include <boost/algorithm/string.hpp>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/types.hpp>

#include <keplerian_toolbox/planet/jpl_low_precision.h>

using namespace bsoncxx;

namespace orbiterkep {

const int max_count_per_problem = 100;

orbiterkep_db::orbiterkep_db():m_inst(mongocxx::instance{}),m_client(mongocxx::client{mongocxx::uri{}}) {
}

pagmo::decision_vector orbiterkep_db::get_stored_solution(const parameters &params, const std::string &problem) {
  using builder::stream::open_array;
  using builder::stream::close_array;
  using builder::stream::open_document;
  using builder::stream::close_document;
  using builder::stream::document;
  using builder::stream::finalize;

  auto collection = m_client["orbiter_kep"]["solutions"];
  
  std::size_t param_hash = boost::hash_value(params);
  boost::hash_combine(param_hash, problem);

  mongocxx::options::find opts;
  opts.sort(document{} << "delta-v" << 1 << finalize);
  opts.limit(1);

  pagmo::decision_vector res;
  auto cursor = collection.find(document{} << "param_hash" << (int64_t)param_hash << finalize);
  for (auto &&doc : cursor) {
    auto vec = doc["decision_vector"].get_value().get_array().value;
    for (auto item : vec) {
      res.push_back(item.get_double().value);
    }
  }

  return res;
}

void orbiterkep_db::store_solution(const parameters &params, const pagmo::problem::transx_solution &solution, const std::string &problem) {
  using builder::stream::open_array;
  using builder::stream::close_array;
  using builder::stream::open_document;
  using builder::stream::close_document;
  using builder::stream::document;
  using builder::stream::finalize;


  auto collection = m_client["orbiter_kep"]["solutions"];

  std::size_t param_hash = boost::hash_value(params);
  boost::hash_combine(param_hash, problem);

  mongocxx::options::find opts;
  opts.sort(document{} << "delta-v" << -1 << finalize);

  document query{};
  query << "param_hash" << (int64_t)param_hash;
  
  int count = collection.count(query << finalize);
  auto cursor = collection.find(query << finalize, opts);
  int i = count - max_count_per_problem;
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

  document << "param_hash" << (int64_t)param_hash;
  
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
