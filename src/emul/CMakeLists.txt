set(LLVM_ROOT $ENV{LLVM_BUILD_ROOT})
if (EXISTS ${LLVM_ROOT}/include/llvm)
  message("LLVM found, compile emulation with LLVM")
  add_subdirectory(llvm)
endif()

add_subdirectory(interpreter)
