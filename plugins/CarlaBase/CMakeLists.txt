# If Carla was not provided by the system, make a dummy library instead
if(LMMS_HAVE_WEAKCARLA)
  SET(CARLA_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/carla/source
    ${CMAKE_CURRENT_SOURCE_DIR}/carla/source/includes
    ${CMAKE_CURRENT_SOURCE_DIR}/carla/source/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/carla/source/backend
  )

  # force "lib" prefix
  IF(LMMS_BUILD_WIN32)
      SET(CMAKE_SHARED_LIBRARY_PREFIX "lib")
  ENDIF()

  SET(CARLA_NATIVE_LIB carla_native-plugin)

  ADD_LIBRARY(${CARLA_NATIVE_LIB} SHARED DummyCarla.cpp)
  target_include_directories(${CARLA_NATIVE_LIB} SYSTEM PUBLIC ${CARLA_INCLUDE_DIRS})
  INSTALL(TARGETS ${CARLA_NATIVE_LIB}
    LIBRARY DESTINATION "${PLUGIN_DIR}/optional"
    RUNTIME DESTINATION "${PLUGIN_DIR}/optional"
  )
  SET(CARLA_LIBRARIES ${CARLA_NATIVE_LIB})
  # Set parent scope variables so carlarack and carlapatchbay can see them
  SET(CARLA_LIBRARIES ${CARLA_LIBRARIES} PARENT_SCOPE)
endif()

if(LMMS_HAVE_CARLA OR LMMS_HAVE_WEAKCARLA)
        # Mimic the missing "config.h"
        SET(CARLA_INCLUDE_DIRS ${CARLA_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/CarlaConfig)
        SET(CARLA_INCLUDE_DIRS ${CARLA_INCLUDE_DIRS} PARENT_SCOPE)

        INCLUDE(BuildPlugin)
        include_directories(SYSTEM ${CARLA_INCLUDE_DIRS})
        LINK_DIRECTORIES(${CARLA_LIBRARY_DIRS})
        LINK_LIBRARIES(${CARLA_LIBRARIES})
        BUILD_PLUGIN(carlabase Carla.cpp Carla.h
            MOCFILES Carla.h
            EMBEDDED_RESOURCES artwork-patchbay.png artwork-rack.png
            EXPORT_BASE_NAME carlabase
            LINK SHARED)
        SET_TARGET_PROPERTIES(carlabase
                PROPERTIES SKIP_BUILD_RPATH TRUE
                BUILD_WITH_INSTALL_RPATH TRUE
                INSTALL_RPATH_USE_LINK_PATH TRUE
                INSTALL_RPATH "${CARLA_RPATH}")
        IF(LMMS_HAVE_WEAKCARLA)
                ADD_DEPENDENCIES(carlabase ${CARLA_NATIVE_LIB})
        ENDIF()
endif()
