cmake_minimum_required(VERSION 3.16)

project(maprecommendationsystem)

find_package(SEAL 4.1.1 REQUIRED)

set(CMAKE_BUILD_TYPE Debug) # lib is release version
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
 " ${CMAKE_MODULE_PATH}")

set(DEP
  SEAL::seal)

# build sealpir lib
add_library(sealpirlib include/pir.hpp src/sealpirlib/pir.cc include/pir_client.hpp
src/sealpirlib/pir_client.cc include/pir_server.hpp src/sealpirlib/pir_server.cc)
target_include_directories(sealpirlib PRIVATE ./include)
target_link_libraries(sealpirlib ${DEP})

# pir query test
add_executable(pirquerytest src/pirquerytest.cc)
target_include_directories(pirquerytest PRIVATE ./include)
target_link_libraries(pirquerytest sealpirlib ${DEP})

# serverplatform
add_executable(serverplatform src/serverplatform.cc)
target_include_directories(serverplatform PRIVATE ./include)
target_link_libraries(serverplatform sealpirlib ${DEP})

# client
add_executable(client src/client.cc)
target_include_directories(client PRIVATE ./include)
target_link_libraries(client PRIVATE sealpirlib ${DEP})

# merchant
add_executable(merchant src/merchant.cc)
target_include_directories(merchant PRIVATE ./include)
target_link_libraries(merchant PRIVATE ${DEP})

# TODO: non-interactive-demo
add_executable(non-interactive-demo src/demo.cc)
target_include_directories(non-interactive-demo PRIVATE ./include)
target_link_libraries(non-interactive-demo PRIVATE ${DEP})

