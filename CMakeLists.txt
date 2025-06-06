cmake_minimum_required(VERSION 3.16)

PROJECT(Zano)


message("OPENSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")    
message("OPENSSL_CRYPTO_LIBRARY: ${OPENSSL_CRYPTO_LIBRARY}")    
message("OPENSSL_SSL_LIBRARY: ${OPENSSL_SSL_LIBRARY}")    




set(VERSION "1.0")

if(POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW)
endif()
if(POLICY CMP0043)
  cmake_policy(SET CMP0074 NEW)
endif()
if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ZANO_BOOST_MIN_VER 1.75)


if(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "Android")
  add_definitions(-DMOBILE_WALLET_BUILD)
  if(CMAKE_SYSTEM_NAME STREQUAL "iOS" )   
    add_definitions(-DIOS_BUILD)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fembed-bitcode")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")			
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_definitions(-DANDROID_BUILD)
    message("Android sdk prefix: ${CMAKE_CXX_ANDROID_TOOLCHAIN_PREFIX}")    
    message("Android CMAKE_CXX_COMPILER prefix: ${CMAKE_CXX_COMPILER}")    
    message("Android ANDROID_LD: ${ANDROID_LD}")
  endif()
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# build types
if (UNIX AND NOT APPLE)
  # single configurations, defaults to Release
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
  endif()
else()
  # multi configurations for MSVC and XCode
  if(CMAKE_SYSTEM_NAME STREQUAL "iOS") 
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Android") 
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
  else()
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
  endif()
endif()
message("Generated with config types: ${CMAKE_CONFIGURATION_TYPES}, and built type: ${CMAKE_BUILD_TYPE}")

enable_testing()

set(OPENSSL_USE_STATIC_LIBS TRUE) # link statically
find_package(OpenSSL REQUIRED)


if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)
endif()

set(USE_PCH FALSE CACHE BOOL "Use shared precompiled headers")
set(DISABLE_TOR FALSE CACHE BOOL "Disable TOR library(and related tor-connect submodule)")
set(TESTNET FALSE CACHE BOOL "Compile for testnet")
set(BUILD_GUI FALSE CACHE BOOL "Build qt-daemon")
set(USE_BITCOIN_SECP256K1_FOR_ECDSA FALSE CACHE BOOL "Use bitcoin-secp256k1 library for validating ECDSA(instead of OpenSSL)")
if(NOT USE_BITCOIN_SECP256K1_FOR_ECDSA)
  add_definitions(-DUSE_OPEN_SSL_FOR_ECDSA)   
endif()


include_directories(src contrib/eos_portable_archive contrib contrib/epee/include contrib/jwt-cpp/include ${OPENSSL_INCLUDE_DIR} "${CMAKE_BINARY_DIR}/version" "${CMAKE_BINARY_DIR}/contrib/zlib")

add_definitions(-DSTATICLIB)

if(TESTNET)
  message("!!!!!! NOTICE: Project is building for TESTNET !!!!!!")  
  add_definitions(-DTESTNET)  
endif()

if(CAKEWALLET)
  message("NOTICE: Building libraries for CAKEWALLET")  
  add_definitions(-DCAKEWALLET)  
  add_definitions(-DDISABLE_PFR_SERIALIZATION_SELFCHECK)  
endif()

set(OPENSSL_USE_STATIC_LIBS TRUE) # link statically
find_package(OpenSSL REQUIRED)

if(DISABLE_TOR)
  message("NOTICE: Building with disabled TOR support!")  
  add_definitions(-DDISABLE_TOR)  
endif()


set(STATIC ${MSVC} CACHE BOOL "Link libraries statically")
if (UNIX AND NOT APPLE)
  # Note that at the time of this writing the -Wstrict-prototypes flag added below will make this fail
  find_package(Threads REQUIRED)
endif()

# TODO(unassigned): expand on types and versions, and then refactor.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CLANG TRUE)
endif()


add_definitions("/DBOOST_NO_CXX98_FUNCTION_BASE")


if(MSVC)
  add_definitions("/D_CRT_SECURE_NO_WARNINGS /D_WIN32_WINNT=0x0600 /DWIN32_LEAN_AND_MEAN /DGTEST_HAS_TR1_TUPLE=0")
  add_compile_options(/bigobj /Zm1000 /Z7 /MP2 /W3 /GS- /wd4996 /wd4503 /wd4345 /wd4091 /FIinline_c.h)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:10485760 /DEBUG dbghelp.lib crypt32.lib")
  if(STATIC)
    foreach(VAR CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE)
      string(REPLACE "/MD" "/MT" ${VAR} "${${VAR}}")
    endforeach()
  endif()
  include_directories(SYSTEM src/platform/msc)
  configure_file(utils/Directory.Build.props.in ${CMAKE_BINARY_DIR}/Directory.Build.props)
else()
  set(ARCH default CACHE STRING "CPU to build for: -march value or default")
  if("${ARCH}" STREQUAL "default")
    set(ARCH_FLAG "")
  else()
    set(ARCH_FLAG "-march=${ARCH}")
  endif()
  set(WARNINGS "-Wall -Wextra -Wpointer-arith -Wvla -Wwrite-strings -Wno-error=extra -Wno-error=deprecated-declarations -Wno-error=sign-compare -Wno-error=strict-aliasing -Wno-error=type-limits -Wno-unused-parameter -Wno-error=unused-variable -Wno-aggregate-return -Wno-comment -Wno-unknown-pragmas -Wno-pragmas")
#  if(NOT APPLE)
#    set(WARNINGS "${WARNINGS} -Werror")
#  endif()
  if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(WARNINGS "${WARNINGS} -Wno-implicit-function-declaration -Wno-shift-count-overflow -Wno-error=mismatched-tags -Wno-error=null-conversion -Wno-overloaded-shift-op-parentheses -Wno-error=shift-count-overflow -Wno-error=tautological-constant-out-of-range-compare -Wno-error=unused-private-field -Wno-error=unneeded-internal-declaration")
  else()
    set(WARNINGS "${WARNINGS} -Wno-error=write-strings -Wlogical-op -Wno-error=maybe-uninitialized")
  endif()

  # Since gcc 4.9 the LTO format is non-standard (slim), so we need the gcc-specific ar and ranlib binaries
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9.0) AND NOT (CMAKE_SYSTEM_NAME STREQUAL "Android"))
    set(CMAKE_AR "gcc-ar")
    set(CMAKE_RANLIB "gcc-ranlib")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT APPLE AND NOT (CMAKE_SYSTEM_NAME STREQUAL "Android"))
    set(CMAKE_AR "llvm-ar")
    set(CMAKE_RANLIB "llvm-ranlib")
  endif()


  if(MINGW)
    set(WARNINGS "${WARNINGS} -Wno-error=unused-value")
    set(MINGW_FLAG "-DWIN32_LEAN_AND_MEAN")
    include_directories(SYSTEM src/platform/mingw)
  else()
    set(MINGW_FLAG "")
  endif()
  if(APPLE)
    set(APPLE_FLAG "-DGTEST_USE_OWN_TR1_TUPLE=1")
  else()
    set(APPLE_FLAG "")
  endif()
  set(C_WARNINGS "-Waggregate-return -Wnested-externs -Wstrict-prototypes -Wno-comment")
  set(CXX_WARNINGS "-Wno-reorder -Wno-missing-field-initializers")
  try_compile(STATIC_ASSERT_RES "${CMAKE_CURRENT_BINARY_DIR}/static-assert" "${CMAKE_CURRENT_SOURCE_DIR}/utils/test-static-assert.c" COMPILE_DEFINITIONS "-std=c++14")
  if(STATIC_ASSERT_RES)
    set(STATIC_ASSERT_FLAG "")
  else()
    set(STATIC_ASSERT_FLAG "-Dstatic_assert=_Static_assert")
  endif()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -D_GNU_SOURCE ${MINGW_FLAG} ${STATIC_ASSERT_FLAG} ${WARNINGS} ${C_WARNINGS} ${ARCH_FLAG}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -ftemplate-depth-1024 -std=c++14 -D_GNU_SOURCE ${APPLE_FLAG} ${MINGW_FLAG} ${WARNINGS} ${CXX_WARNINGS} ${ARCH_FLAG}")
  if (NOT APPLE AND NOT MSVC)
    if (CLANG)
      set(LLVM_USE_LINKER "gold")
    else()
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")
    endif()
  endif()
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT (CMAKE_C_COMPILER_VERSION VERSION_LESS 4.8))
    set(DEBUG_FLAGS "-g3 -O0") #set(DEBUG_FLAGS "-g3 -Og")
  else()
    set(DEBUG_FLAGS "-g3 -O0")
  endif()
  set(RELEASE_FLAGS "-Ofast -DNDEBUG -Wno-unused-variable")

  if(NOT APPLE AND NOT (CMAKE_SYSTEM_NAME STREQUAL "Android"))
    set(RELEASE_FLAGS "${RELEASE_FLAGS} -flto")
  endif()
  #if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT MINGW)
  #  set(RELEASE_FLAGS "${RELEASE_FLAGS} -fno-fat-lto-objects")
  #endif()
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${DEBUG_FLAGS}")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${DEBUG_FLAGS}")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${RELEASE_FLAGS}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${RELEASE_FLAGS}")
  if(STATIC)
    if(APPLE)
      message(SEND_ERROR "Static build is not supported on MacOS X")
    else()
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()
  endif()
endif()


# always use local Boost installation rather than the system-wide (unless ZANO_USE_SYSTEM_BOOST is defined for some reason, which is not recommended)
if(NOT DEFINED ENV{ZANO_USE_SYSTEM_BOOST})
  set(Boost_NO_SYSTEM_PATHS ON)
endif()

if(MSVC)
  set(Boost_USE_STATIC_LIBS ON)
endif()

if(STATIC)  
  set(Boost_USE_STATIC_LIBS ON)  
  set(Boost_USE_STATIC_RUNTIME ON)
endif()

message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")    
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
  set(CMAKE_OSX_DEPLOYMENT_TARGET 12.00)
  if(NOT DEFINED SKIP_BOOST_FATLIB_LIB OR NOT SKIP_BOOST_FATLIB_LIB)
	message("Ios: libboost.a included as library")    
	set(Boost_LIBRARIES "libboost.a")
  else()
	message("Ios: libboost.a not included as library")    
  endif()
  #workaround for new XCode 12 policy for builds(now it includes a slice for the "arm64" when builds for simulator)
  set(__iphoneos_archs "arm64")
  #set(__iphonesimulator_archs "arm64,x86_64")
  set(CMAKE_XCODE_ATTRIBUTE_ARCHS[sdk=iphoneos*] "${__iphoneos_archs}")
  set(CMAKE_XCODE_ATTRIBUTE_VALID_ARCHS[sdk=iphoneos*] "${__iphoneos_archs}")
  #set(CMAKE_XCODE_ATTRIBUTE_ARCHS[sdk=iphonesimulator*] "${__iphonesimulator_archs}")
  #set(CMAKE_XCODE_ATTRIBUTE_VALID_ARCHS[sdk=iphonesimulator*] "${__iphonesimulator_archs}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
  if(CAKEWALLET)
    find_package(Boost ${ZANO_BOOST_MIN_VER} REQUIRED COMPONENTS system filesystem thread timer date_time chrono regex serialization atomic program_options locale)
  else()
    set(Boost_LIBRARY_DIRS "${Boost_LIBRARY_DIRS}/${CMAKE_ANDROID_ARCH_ABI}/")
    set(Boost_LIBRARIES "${Boost_LIBRARY_DIRS}libboost_system.a;${Boost_LIBRARY_DIRS}libboost_filesystem.a;${Boost_LIBRARY_DIRS}libboost_thread.a;${Boost_LIBRARY_DIRS}libboost_timer.a;${Boost_LIBRARY_DIRS}libboost_date_time.a;${Boost_LIBRARY_DIRS}libboost_chrono.a;${Boost_LIBRARY_DIRS}libboost_regex.a;${Boost_LIBRARY_DIRS}libboost_serialization.a;${Boost_LIBRARY_DIRS}libboost_atomic.a;${Boost_LIBRARY_DIRS}libboost_program_options.a")
  endif()  
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fPIC")
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fPIC")
elseif(APPLE)
  find_package(Boost ${ZANO_BOOST_MIN_VER} REQUIRED COMPONENTS system filesystem thread timer date_time chrono regex serialization atomic program_options locale)
else()
  find_package(Boost ${ZANO_BOOST_MIN_VER} REQUIRED COMPONENTS system filesystem thread timer date_time chrono regex serialization atomic program_options locale log)
endif()



message(STATUS "Boost: ${Boost_VERSION} from ${Boost_LIBRARY_DIRS}")


include_directories(SYSTEM ${Boost_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/contrib/ethereum/libethash)
if(MINGW)
  set(Boost_LIBRARIES "${Boost_LIBRARIES};ws2_32;mswsock")
elseif(NOT MSVC)
  if(NOT APPLE AND NOT CAKEWALLET)
    set(Boost_LIBRARIES "${Boost_LIBRARIES};")
    if(STATIC) 
      message("NOTICE: Including static ICU libraries")  
      set(Boost_LIBRARIES "${Boost_LIBRARIES};icui18n.a;icuuc.a;icudata.a;dl")
    endif()
  endif()
endif()

if(BUILD_GUI)
  cmake_minimum_required(VERSION 3.1) # <-- this is important for Linux GUI build, don't touch it -- sowle
  find_package(Qt5Widgets REQUIRED)  
  find_package(Qt5WebEngineWidgets REQUIRED)  
  find_package(Qt5WebChannel REQUIRED)  
endif()

set(COMMIT_ID_IN_VERSION ON CACHE BOOL "Include commit ID in version")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/version")
if (NOT COMMIT_ID_IN_VERSION)
  set(VERSION "${VERSION}-unknown")
  configure_file("src/version.h.in" "version/version.h")
  add_custom_target(version ALL)
elseif(DEFINED COMMIT)
  string(REPLACE "." "\\." VERSION_RE "${VERSION}")
  if(NOT REFS MATCHES "(\\(|, )tag: v${VERSION_RE}(\\)|, )")
    set(VERSION "${VERSION}-g${COMMIT}")
  endif()
  configure_file("src/version.h.in" "version/version.h")
  add_custom_target(version ALL)
else()
  find_package(Git QUIET)
  if(Git_FOUND OR GIT_FOUND)
    message(STATUS "Found Git: ${GIT_EXECUTABLE}")
    add_custom_target(version ALL "${CMAKE_COMMAND}" "-D" "VERSION=${VERSION}" "-D" "GIT=${GIT_EXECUTABLE}" "-D" "TO=${CMAKE_BINARY_DIR}/version/version.h" "-P" "src/version.cmake" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  else()
    message(STATUS "WARNING: Git was not found!")
    set(VERSION "${VERSION}-unknown")
    configure_file("src/version.h.in" "version/version.h")
    add_custom_target(version ALL)
  endif()
endif()

set(BUILD_TESTS FALSE CACHE BOOL "Build Zano tests")
set(DISABLE_MDBX FALSE CACHE BOOL "Exclude mdbx from build(need for a first time)") 
if(NOT DISABLE_MDBX)
   add_definitions(-DENABLED_ENGINE_MDBX)
endif()

  
add_subdirectory(contrib)
add_subdirectory(src)

if (BUILD_TESTS)
  add_subdirectory(tests)
endif()
