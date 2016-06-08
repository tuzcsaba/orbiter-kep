#include "db/orbiter_db.h"

#ifdef BUILD_MONGODB

#include <bcon.h>
#include <mongoc.h>

#include <boost/algorithm/string.hpp>

#include <keplerian_toolbox/planet/jpl_low_precision.h>


namespace orbiterkep {

const int max_count_per_problem = 25;

orbiterkep_db::orbiterkep_db() {
  mongoc_init();
  m_client = mongoc_client_new("mongodb://127.0.0.1:27017");
}

orbiterkep_db::~orbiterkep_db() {
  if (m_client != 0) {
    mongoc_client_destroy(m_client);
  }
  mongoc_cleanup();
}

pagmo::decision_vector orbiterkep_db::get_stored_solution(const Parameters &params, const std::string &problem) {

  auto collection = mongoc_client_get_collection(m_client, "orbiter_kep", "solutions");
  
  std::size_t param_hash = boost::hash_value(params);
  boost::hash_combine(param_hash, problem);


  auto query = BCON_NEW("$query", "{", "param_hash", BCON_INT64(param_hash), "problem", BCON_UTF8(params.problem().c_str()), "}",
                        "$orderby", "{", "delta-v", BCON_INT32 (1), "}");

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
            double v = bson_iter_double(&item_iter);
            res.push_back(v);
          }
        }
      }
    }
  }

  bson_destroy(query);

  mongoc_cursor_destroy(cursor);

  mongoc_collection_destroy(collection);

  return res;
}

void orbiterkep_db::store_solution(const Parameters &params, const orbiterkep::TransXSolution &solution, const std::string &problem) {

  bson_error_t error;

  auto collection = mongoc_client_get_collection(m_client, "orbiter_kep", "solutions");

  std::size_t param_hash = boost::hash_value(params);
  boost::hash_combine(param_hash, problem);

  bson_t * countQuery = BCON_NEW("param_hash", BCON_INT64(param_hash));

  bson_t * query = BCON_NEW("$query", "{", "param_hash", BCON_INT64(param_hash), "}",
                        "$orderby", "{", "delta-v", BCON_INT32(-1), "}");
  
  int count = mongoc_collection_count(collection, MONGOC_QUERY_NONE, countQuery, 0, 0, NULL, &error);
  auto cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 0, 0, query, NULL, NULL);
  int i = count - max_count_per_problem;

  bson_destroy(countQuery);

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
  bson_destroy(query);
  mongoc_cursor_destroy(cursor);

  bson_oid_t oid;
  bson_oid_init(&oid, NULL);

  auto launch = solution.times().times(0);

  auto document = bson_new();
  BSON_APPEND_OID(document, "_id", &oid);
  BSON_APPEND_INT64(document, "param_hash", param_hash);
  
  bson_t child;
  bson_append_array_begin(document, "planets", -1, &child);
  for (int i = 0; i < params.planets_size(); ++i) {
    auto planet = params.planets(i);
    char key[15];
    sprintf(key, "%d", i + 1);
    BSON_APPEND_UTF8(&child, key, planet.c_str());
  } 
  bson_append_array_end(document, &child);
  

  BSON_APPEND_UTF8(document, "problem", solution.problem().c_str());
  std::stringstream ss;
  ss.str("");
  ss.clear();
  ss << kep_toolbox::epoch(params.t0().lb(), kep_toolbox::epoch::type::MJD);
  auto t0_min = ss.str();

  ss.str("");
  ss.clear();
  ss << kep_toolbox::epoch(params.t0().ub(), kep_toolbox::epoch::type::MJD);
  auto t0_max = ss.str();
  BSON_APPEND_DOCUMENT(document, "t0", BCON_NEW("min", BCON_UTF8(t0_min.c_str()), "max", BCON_UTF8(t0_max.c_str())));
  BSON_APPEND_DOCUMENT(document, "tof", BCON_NEW("min", BCON_DOUBLE(params.tof().lb()), "max", BCON_DOUBLE(params.tof().ub())));
  BSON_APPEND_DOCUMENT(document, "vinf", BCON_NEW("min", BCON_DOUBLE(params.vinf().lb()), "max", BCON_DOUBLE(params.vinf().ub())));
  BSON_APPEND_DOUBLE(document, "launch_mjd", launch);
  BSON_APPEND_DOUBLE(document, "eject_altitude", params.dep_altitude());
  BSON_APPEND_DOUBLE(document, "target_altitude", params.arr_altitude());
  BSON_APPEND_DOUBLE(document, "delta-v", solution.fuel_cost());

  ss.str("");
  ss.clear();
  ss << solution;
  BSON_APPEND_UTF8(document, "transx_plan", ss.str().c_str());

  bson_append_array_begin(document, "decision_vector", -1, &child);
  for (int i = 0; i < solution.x().size(); ++i) {
    auto value = solution.x(i);
    char key[15];
    sprintf(key, "%d", i + 1);
    BSON_APPEND_DOUBLE(&child, key, value);
  }
  bson_append_array_end(document, &child);

  if (!mongoc_collection_insert(collection, MONGOC_INSERT_NONE, document, NULL, &error)) {
    std::cout << error.message << std::endl;
  }

  bson_destroy(document);
  mongoc_collection_destroy(collection);
};

} // namespaces

#endif
