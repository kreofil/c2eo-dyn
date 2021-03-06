cmake_minimum_required(VERSION 3.18)


project(c2eo)

set( LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  Option
  Support
  )

# llvm_config
#set(PATH_TO_LLVM_SOURCE "../../../llvm-clang")
#set(PATH_TO_LLVM_SOURCE "~/llvm-clang")
set(PATH_TO_LLVM_SOURCE "~/llvm-clang-12")
#set(PATH_TO_LLVM_SOURCE "~/llvm-clang-14")


# set(CMAKE_C_COMPILER путь/до/компилятора)

set(PATH_TO_LLVM_BUILD "${PATH_TO_LLVM_SOURCE}/build")
include_directories("${PATH_TO_LLVM_SOURCE}/clang/include")
include_directories("${PATH_TO_LLVM_SOURCE}/llvm/include")
include_directories("${PATH_TO_LLVM_BUILD}/tools/clang/include")
include_directories("${PATH_TO_LLVM_BUILD}/include")
link_directories("${PATH_TO_LLVM_BUILD}/lib/")


# llvm_config for yarrya:
# include_directories("${PATH_TO_LLVM_SOURCE}/clang-include")
# include_directories("${PATH_TO_LLVM_SOURCE}/llvm-include")
# include_directories("${PATH_TO_LLVM_SOURCE}/tools-clang-include")
# include_directories("${PATH_TO_LLVM_SOURCE}/build-include")
# link_directories("~/Polystat/llvm-clang-12/build/lib")
# link_directories("/usr/lib")


set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fno-rtti)

# Source files list
set(SOURCE_FILES
  # Headers
  vardecl.h
  analyzers.h
  matchers.h
  util.h
  memory_manager.h
  unit_transpiler.h
  eo_object.h
  function_manager.h
  transpile_helper.h
  aliases.h
  tracer.h

  # Sources
  main.cpp
  vardecl.cpp
  analyzers.cpp
  matchers.cpp
  util.cpp
  memory_manager.cpp
  unit_transpiler.cpp
  eo_object.cpp
  function_manager.cpp
  transpile_helper.cpp
  tracer.cpp
  )

add_executable(c2eo ${SOURCE_FILES})

# llvm libraries
target_link_libraries(c2eo
  PRIVATE
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangRewriteFrontend
  clangSerialization
  clangStaticAnalyzerFrontend
  clangTooling
  clangToolingSyntax

  #clangBasic
  #clangFrontend
  #clangAST
  #clangSerialization
  clangASTMatchers
  #clangTooling
  LLVMSupport
  LLVMFrontendOpenMP
  )

