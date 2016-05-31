
set(ORBITERKEP_SRC_LIST 
  ${ORBITERKEP_SRC_LIST}
  ${CMAKE_CURRENT_SOURCE_DIR}/optimise.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/problems/transx_problem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/problems/mga_transx.cpp
)

add_library(orbiterkep_static ${ORBITERKEP_SRC_LIST})


