find_package(TinyXML REQUIRED)

include_directories(${OPENGL_INCLUDE_DIR})
link_directories(${OPENGL_LIBRARY_DIR})
include_directories(${GLEW_INCLUDE_DIR})
link_directories(${GLEW_LIBRARY_DIR})
include_directories(${OPENAL_INCLUDE_DIR})
include_directories(${OGGVORBIS_INCLUDE_DIR})
include_directories(${ASSIMP_INCLUDE_DIR})
link_directories(${ASSIMP_LIBRARY_DIR})
include_directories(${SDL2_INCLUDE_DIRS})
link_directories(${SDL2_LIBRARY_DIRS})
include_directories(${SDL2_IMAGE_INCLUDE_DIRS})
link_directories(${SDL2_IMAGE_LIBRARY_DIRS})
include_directories(${SDL2_TTF_INCLUDE_DIRS})
link_directories(${SDL2_TTF_LIBRARY_DIRS})
include_directories(${SDL2_NET_INCLUDE_DIRS})
link_directories(${SDL2_NET_LIBRARY_DIRS})
include_directories(${TINYXML_INCLUDE_DIR})
link_directories(${TINYXML_LIBRARY_DIR})
include_directories(${TINY_SOURCE_DIR})
include_directories(${TINY_BINARY_DIR})

set(MINIONS_LIBS 
              tinygame
              ${ASSIMP_LIBRARIES}
              ${OPENGL_LIBRARY}
              ${GLEW_LIBRARY}
              ${OPENAL_LIBRARY}
              ${OGGVORBIS_LIBRARIES}
              ${SDL2_LIBRARIES}
              ${SDL2IMAGE_LIBRARIES}
              ${SDL2TTF_LIBRARIES}
              ${SDL2NET_LIBRARIES}
              ${TINYXML_LIBRARIES})

add_executable(minions src/minions.cpp
                     src/terrain.cpp
                     src/forest.cpp
                     src/minion_type.cpp
                     src/game.cpp)

target_link_libraries(minions ${MINIONS_LIBS})

