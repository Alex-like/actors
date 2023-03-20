cmake_minimum_required(VERSION 3.22)
project(Actors)

set(CMAKE_CXX_STANDARD 20)

# Add path to OpenSSL
set(OPENSSL_ROOT_DIR "/usr/local/Cellar/openssl@3/3.0.8/")
set(OPENSSL_INCLUDE_DIR "/usr/local/Cellar/openssl@3/3.0.8/include")
set(OPENSSL_LIBRARIES "/usr/local/Cellar/openssl@3/3.0.8/bin")
set(OPENSSL_CRYPTO_LIBRARY "/usr/local/Cellar/openssl@3/3.0.8/lib/libcrypto.dylib")
# GoogleTests
include(FetchContent)
FetchContent_Declare(
        googletest
        # Specify the commit you depend on and update it regularly.
        URL https://github.com/google/googletest/archive/cead3d57c93ff8c4e5c1bbae57a5c0b0b0f6e168.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# C++ Actor Framework
FetchContent_Declare(
        CAF
        URL https://github.com/actor-framework/actor-framework/archive/d9aca2e1357cdbb932dea9a70d757b03b81abd10.zip
)
FetchContent_MakeAvailable(googletest CAF)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/src )
file( GLOB TargetSources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp)
file( GLOB TestSources ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp)

add_executable( actors_main main.cpp ${TargetSources} )
target_link_libraries( actors_main PRIVATE CAF::internal CAF::core )

add_executable( actors_tests ${TestSources} )
target_link_libraries( actors_tests PRIVATE CAF::internal CAF::core CAF::test gtest )
