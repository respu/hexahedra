IF(ES_LIBRARY AND ES_INCLUDE_DIR)
  # in cache already
  SET(ES_FIND_QUIETLY TRUE)
ENDIF(ES_LIBRARY AND ES_INCLUDE_DIR)

IF(NOT ES_INCLUDE_DIR)
    find_path(ES_INCLUDE_DIR
      es/storage.hpp
      "${CMAKE_SOURCE_DIR}/../es/src")
ENDIF()

IF(NOT ES_LIBRARY)
    find_library(ES_LIBRARY es
      "${CMAKE_SOURCE_DIR}/../es"
      es)
ENDIF()


IF(ES_LIBRARY AND ES_INCLUDE_DIR)
  SET(ES_FOUND "YES")
  IF(NOT ES_FIND_QUIETLY)
    MESSAGE(STATUS "Found Entity System library: ${ES_LIBRARY}")
  ENDIF(NOT ES_FIND_QUIETLY)
ELSE(ES_LIBRARY AND ES_INCLUDE_DIR)
  IF(NOT ES_FIND_QUIETLY)
    MESSAGE(STATUS "Warning: Unable to find Entity System library! Get it from http://github.com/Nocte-/es")
  ENDIF(NOT ES_FIND_QUIETLY)
ENDIF(ES_LIBRARY AND ES_INCLUDE_DIR)
