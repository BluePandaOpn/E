cmake_minimum_required(VERSION 3.16)
project(epp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(epp
    src/cli/main.cpp
    src/core/compiler.cpp
    src/core/version_manager.cpp
    src/core/lexer.cpp
    src/core/parser.cpp
    src/core/runtime.cpp
)

set(EPP_LIB_ROOT "")
if((EXISTS "${CMAKE_SOURCE_DIR}/lib/lib/native_std_cpp.cpp" OR
    EXISTS "${CMAKE_SOURCE_DIR}/lib/lib/native_std_cpp/native_std_cpp.cpp") AND
   EXISTS "${CMAKE_SOURCE_DIR}/lib/include/epp_native.h")
    set(EPP_LIB_ROOT "${CMAKE_SOURCE_DIR}/lib")
elseif((EXISTS "${CMAKE_SOURCE_DIR}/../lib/lib/native_std_cpp.cpp" OR
        EXISTS "${CMAKE_SOURCE_DIR}/../lib/lib/native_std_cpp/native_std_cpp.cpp") AND
       EXISTS "${CMAKE_SOURCE_DIR}/../lib/include/epp_native.h")
    set(EPP_LIB_ROOT "${CMAKE_SOURCE_DIR}/../lib")
else()
    message(FATAL_ERROR
        "No se encontro la libreria nativa. Esperado en "
        "'${CMAKE_SOURCE_DIR}/lib' o '${CMAKE_SOURCE_DIR}/../lib'.")
endif()

set(EPP_NATIVE_STD_CPP_SRC "")
if(EXISTS "${EPP_LIB_ROOT}/lib/native_std_cpp.cpp")
    set(EPP_NATIVE_STD_CPP_SRC "${EPP_LIB_ROOT}/lib/native_std_cpp.cpp")
elseif(EXISTS "${EPP_LIB_ROOT}/lib/native_std_cpp/native_std_cpp.cpp")
    set(EPP_NATIVE_STD_CPP_SRC "${EPP_LIB_ROOT}/lib/native_std_cpp/native_std_cpp.cpp")
else()
    message(FATAL_ERROR "No se encontro fuente native_std_cpp.cpp en ${EPP_LIB_ROOT}/lib")
endif()

target_include_directories(epp PRIVATE
    src/core
    "${EPP_LIB_ROOT}/include"
)

add_executable(did
    src/cli/did_main.cpp
)

target_include_directories(did PRIVATE src/core)

add_library(native_std_cpp SHARED
    "${EPP_NATIVE_STD_CPP_SRC}"
)

target_include_directories(native_std_cpp PRIVATE "${EPP_LIB_ROOT}/include")
add_custom_command(TARGET native_std_cpp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:native_std_cpp>
            ${EPP_LIB_ROOT}/lib/native_std_cpp.dll
    COMMENT "Copiando native_std_cpp.dll a lib/"
)

if(MSVC)
    target_link_libraries(epp PRIVATE ws2_32)

    target_compile_options(epp PRIVATE
        $<$<CONFIG:Release>:/O2 /Ob2 /Oi /Ot /GL /Gw /Gy>
    )
    target_link_options(epp PRIVATE
        $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
    )

    target_compile_options(native_std_cpp PRIVATE
        $<$<CONFIG:Release>:/O2 /Ob2 /Oi /Ot /GL /Gw /Gy>
    )
    target_link_options(native_std_cpp PRIVATE
        $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF>
    )
    target_link_libraries(native_std_cpp PRIVATE ws2_32)
else()
    target_compile_options(epp PRIVATE
        $<$<CONFIG:Release>:-O3 -flto -DNDEBUG>
    )
    target_link_options(epp PRIVATE
        $<$<CONFIG:Release>:-flto>
    )

    target_compile_options(native_std_cpp PRIVATE
        $<$<CONFIG:Release>:-O3 -flto -DNDEBUG>
    )
    target_link_options(native_std_cpp PRIVATE
        $<$<CONFIG:Release>:-flto>
    )
endif()

