#include "orbiter_db.h"

#ifdef BUILD_MONGODB

#include <bcon.h>
#include <mongoc.h>

#include <boost/algorithm/string.hpp>

#include <keplerian_toolbox/planet/jpl_low_precision.h>


namespace orbiterkep {

const int max_count_per_problem = 100;

orbiterkep_db::orbiterkep_db():m_client(mongoc_client_new("mongodb://127.0.0.1:27017")) {
}

orbiterkep_db::~orbiterkep_db() {
  mongoc_client_destroy(m_client); m_client = 0;
}

pagmo::decision_vector orbiterkep_db::get_stored_solution(const parameters &params, const std::string &problem) {

  auto collection = mongoc_client_get_collection(m_client, "orbiter_kep", "solutions");
  
  std::size_t param_hash = boost::hash_value(params);
  boost::hash_combine(param_hash, problem);


  auto query = BCON_NEW("$query", "{", "param_hash", BCON_INT64(param_hash), "}",
                        "sort", "{", "delta-v", BCON_INT32 (-1), "}");

  pagmo::decision_vector res;
  auto cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 0, 0, query, NULL, NULL);

  const bson_t * doc;
  while (mongoc_cursor_more(cursor) && mongoc_cursor_next(cursor, &doc)) {
    bson_iter_t iter;
    bson_iter_t decision_vec;

    if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "decision_vector", &decision_vec) && BSON_ITER_HOLDS_ARRAY(&decision_vec)) {
      uint32_t vec_dim;
      const uint8_t *vec;
      bson_t vecItems; 
      bson_iter_t item_iter;
      bson_iter_array(&decision_vec, &vec_dim, &vec);
      if (bson_init_static(&vecItems, vec, vec_dim)) {
        if (bson_iter_init(&item_iter, &vecItems)) {
          while (bson_iter_next(&item_iter)) {
            res.push_back(bson_iter_double(&item_iter));
          }
        }
      }
    }
  }

  return res;
}

void orbiterkep_db::store_solution(const parameters &params, const pagmo::problem::TransXSolution &solution, const std::string &problem) {

  bson_error_t error;

  auto collection = mongoc_client_get_collection(m_client, "orbiter_kep", "solutions");

  std::size_t param_hash = boost::hash_value(params);
  boost::hash_combine(param_hash, problem);

  auto query = BCON_NEW("$query", "{", "param_hash", param_hash, "}",
                        "$sort", "{", "delta-v", BCON_INT32(-1), "}");
  
  
  int count = mongoc_collection_count(collection, MONGOC_QUERY_NONE, query, 0, 0, NULL, &error);

  auto cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 0, 0, query, NULL, NULL);
  int i = count - max_count_per_problem;

  const bson_t * doc;
  while (mongoc_cursor_more(cursor) && mongoc_cursor_next(cursor, &doc)) {
    if (i <= 0) break;

    bson_iter_t iter;

    if (!bson_iter_init(&iter, doc)) return;
    
    bson_iter_t id_iter;
    bson_iter_t deltav_iter;

    bson_iter_find_descendant(&iter, "_id", &id_iter);
    auto id = bson_iter_oid(&id_iter);
    bson_iter_find_descendant(&iter, "delta-v", &deltav_iter); 
    auto deltaV = bson_iter_double(&deltav_iter);

    if (deltaV < solution.fuel_cost()) {
      return;
    }
    
    bson_t * toDel = bson_new();
    BSON_APPEND_OID(toDel, "_id", id);

    mongoc_collection_remove(collection, MONGOC_REMOVE_SINGLE_REMOVE, toDel, NULL, &error);

    bson_destroy(toDel);

    --i;
  }

  bson_oid_t oid;
  bson_oid_init(&oid, NULL);

    j
    bson_t * decisionVec = bson_new();
  auto launch = solution.times().times(0);

  auto document = bson_new();
  bson_append_oid(document, &oid);
  bson_append_int64(document, "param_hash", param_hash);
  
  bson_t child;
  bson_append_array_begin(document, "planets", -1, &child);
  for (int i = 0; i < params.planets.size(); ++i) {
    auto planet = params.planets[i];
    char key[15];
    sprintf(str, "%d", i);
    bson_append_utf8(&child, str, -1, planet->get_name().c_str(), -1);
  } 
  bson_append_array_end(document, &child);

  bson_append_utf8(document, "problem", -1, solution.problem().c_str(), -1);
  bson_append_double(document, "launch_mjd", launch);
  bson_append_double(document, "eject_altitude", params.dep_altitude);
  bson_append_double(document, "target_altitude", params.arr_altitude);
  bson_append_double(document, "delta-v", solution.fuel_cost());

  std::stringstream ss; ss << solution;
  bson_append_utf8(document, "transx_plan", -1, ss.str().c_str(), -1);

  bson_append_array_begin(document, "decision_vector", -1, &child);
  for (int i = 0; i < solution.x().size(); ++i) {
    auto value = solution.x(i);
    char key[15];
    sprintf(str, "%d", i);
    bson_append_double(document, str, value);
  }
  bson_append_array_end(document, &child);

  if (!mongoc_collection_insert(collection, MONGOC_INSERT_NONE, document, NULL, &error)) {
    std::cout << error.message << std::endl;
  }
};

} // namespaces

#endif
