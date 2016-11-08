cmake_minimum_required(VERSION 3.6)
project(indriserver)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(Indri REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Boost COMPONENTS system program_options locale)
find_package(SQLite3 REQUIRED)

include_directories("${INDRI_INCLUDE_DIR}" "${Boost_INCLUDE_DIRS}"
        "${CMAKE_SOURCE_DIR}/websocketpp" "${SQLITE3_INCLUDE_DIR}")

set(SOURCE_FILES main.cpp handler.cpp handler.h QueryHandler.cpp QueryHandler.h utils.cpp utils.h Leaderboard.cpp Leaderboard.h)


add_executable(indriserver ${SOURCE_FILES})
set_property(TARGET indriserver PROPERTY CXX_STANDARD 11)
target_link_libraries(indriserver ${INDRI_LIBRARY} ${ZLIB_LIBRARY}
        ${Boost_LIBRARIES} ${SQLITE3_LIBRARIES})
