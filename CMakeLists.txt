project(petway)

set(CMAKE_C_COMPILER clang-16)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-gnu -Wno-unused-parameter -Wno-format-pedantic)

if(DEFINED ENV{DEBUG})
    add_compile_options(-g)
else()
    add_compile_options(-O3)
endif()

find_package(ICU COMPONENTS uc)

add_library(petway STATIC
    src/pw_args.c
    src/pw_array.c
    src/pw_array_iterator.c
    src/pw_assert.c
    src/pw_charptr.c
    src/pw_compound.c
    src/pw_datetime.c
    src/pw_dump.c
    src/pw_file.c
    src/pw_hash.c
    src/pw_interfaces.c
    src/pw_iterator.c
    src/pw_map.c
    src/pw_netutils.c
    src/pw_parse.c
    src/pw_socket.c
    src/pw_status.c
    src/pw_string.c
    src/pw_string_io.c
    src/pw_struct.c
    src/pw_to_json.c
    src/pw_types.c
)

target_include_directories(petway PUBLIC . include libpussy)
target_link_libraries(petway ${CMAKE_SOURCE_DIR}/libpussy/libpussy.a)

# test

add_executable(test_pw test/test_pw.c)

target_link_libraries(test_pw petway)

if(DEFINED ICU_FOUND AND NOT DEFINED ENV{PW_WITHOUT_ICU})
    target_link_libraries(test_pw ICU::uc)
endif()

# common definitions

set(common_defs_targets petway test_pw)

foreach(TARGET ${common_defs_targets})

    if(DEFINED ENV{DEBUG})
        target_compile_definitions(${TARGET} PUBLIC DEBUG)
    endif()

    if(DEFINED ICU_FOUND AND NOT DEFINED ENV{PW_WITHOUT_ICU})
        target_compile_definitions(${TARGET} PUBLIC PW_WITH_ICU)
    endif()

endforeach(TARGET)
