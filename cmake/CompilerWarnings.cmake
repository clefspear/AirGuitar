function(set_project_warnings target)
  option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

  set(CLANG_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Werror=return-type
    -Werror=uninitialized
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Woverloaded-virtual
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wlogical-op
  )

  if(WARNINGS_AS_ERRORS)
    set(CLANG_WARNINGS ${CLANG_WARNINGS} -Werror)
  endif()

  target_compile_options(${target} PRIVATE ${CLANG_WARNINGS})
endfunction()
