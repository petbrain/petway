project(petway)

# set(CMAKE_VERBOSE_MAKEFILE ON)
#set(CMAKE_C_COMPILER clang)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unused-parameter)

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
    src/pw_compound.c
    src/pw_datetime.c
    src/pw_dump.c
    src/pw_file.c
    src/pw_hash.c
    src/pw_helpers.c
    src/pw_interfaces.c
    src/pw_iterator.c
    src/pw_map.c
    src/pw_netutils.c
    src/pw_parse.c
    src/pw_socket.c
    src/pw_status.c
    src/pw_string_io.c
    src/pw_struct.c
    src/pw_to_json.c
    src/pw_task.c
    src/pw_types.c
    src/string/append.c
    src/string/char_at.c
    src/string/copy.c
    src/string/create.c
    src/string/cstring.c
    src/string/equal_z.c
    src/string/erase.c
    src/string/hash.c
    src/string/insert_many.c
    src/string/is_ascii_digit.c
    src/string/isdigit.c
    src/string/isspace.c
    src/string/panic.c
    src/string/pw_string_type.c
    src/string/rsplit_chr.c
    src/string/skip_chars.c
    src/string/skip_spaces.c
    src/string/split_chr.c
    src/string/strcat_va.c
    src/string/strchr.c
    src/string/strchr2.c
    src/string/strrchr2.c
    src/string/strstr.c
    src/string/strstri.c
    src/string/strstrz.c
    src/string/substr.c
    src/string/substreq.c
    src/string/substreqz.c
    src/string/trim.c
    src/string/truncate.c
    src/string/upper_lower.c
    src/string/utf.c
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
