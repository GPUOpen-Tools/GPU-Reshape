
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

function(TargetEnableAsan TARGET)
    get_target_property(Imported ${TARGET} IMPORTED)
    if (${Imported})
        return()
    endif ()

    # TODO: Detect local environment
    target_link_directories(${TARGET} PRIVATE "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.28.29333/lib/x64")
    target_link_libraries(${TARGET} PRIVATE clang_rt.asan_dynamic-x86_64 clang_rt.asan_dynamic_runtime_thunk-x86_64)
    target_link_options(${TARGET} PRIVATE /wholearchive:clang_rt.asan_dynamic_runtime_thunk-x86_64.lib)
endfunction()

function(add_library NAME)
    _add_library(${NAME} ${ARGN})
    TargetEnableAsan(${NAME})
endfunction(add_library)

function(add_executable NAME)
    _add_executable(${NAME} ${ARGN})
    TargetEnableAsan(${NAME})
endfunction(add_executable)
