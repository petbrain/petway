#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/pw.h"
#include "include/pw_args.h"
#include "include/pw_datetime.h"
#include "include/pw_netutils.h"
#include "include/pw_socket.h"
#include "include/pw_to_json.h"
#include "include/pw_utf.h"
#include "src/pw_string_internal.h"

int num_tests = 0;
int num_ok = 0;
int num_fail = 0;
//bool print_ok = true;
bool print_ok = false;

#define TEST(condition) \
    do {  \
        if (condition) {  \
            num_ok++;  \
            if (print_ok) fprintf(stderr, "OK: " #condition "\n");  \
        } else {  \
            num_fail++;  \
            fprintf(stderr, "FAILED at line %d: " #condition "\n", __LINE__);  \
        }  \
        num_tests++;  \
    } while (false)

#define panic()  \
    do {  \
        fprintf(stderr, "PANIC: %s:%d\n", __FILE__, __LINE__);  \
        pw_print_status(stderr, &current_task->status);  \
        abort();  \
    } while (false)

void test_icu()
{
#   ifdef PW_WITH_ICU
        puts("With ICU");
        TEST(pw_isspace(' '));
        TEST(pw_isspace(0x2003));
#   else
        puts("Without ICU");
        TEST(pw_isspace(' '));
        TEST(!pw_isspace(0x2003));
#   endif
}

void test_integral_types()
{
    // generics test
    TEST(strcmp(pw_get_type_name((char) PwTypeId_Bool), "Bool") == 0);
    TEST(strcmp(pw_get_type_name((int) PwTypeId_Signed), "Signed") == 0);
    TEST(strcmp(pw_get_type_name((unsigned long long) PwTypeId_Float), "Float") == 0);

    // Null values
    PwValue null_1 = PW_NULL;
    TEST(pw_is_null(&null_1));

    TEST(strcmp(pw_get_type_name(&null_1), "Null") == 0);  // generics test

    // Bool values
    PwValue bool_true  = PwBool(true);
    PwValue bool_false = PwBool(false);
    TEST(pw_is_bool(&bool_true));
    TEST(pw_is_bool(&bool_false));

    // Int values
    PwValue int_0 = PwSigned(0);
    PwValue int_1 = PwSigned(1);
    PwValue int_neg1 = PwSigned(-1);
    TEST(pw_is_int(&int_0));
    TEST(pw_is_int(&int_1));
    TEST(pw_is_signed(&int_1));
    TEST(pw_is_signed(&int_neg1));
    TEST(pw_equal(&int_0, 0));
    TEST(!pw_equal(&int_0, 1));
    TEST(pw_equal(&int_1, 1));
    TEST(pw_equal(&int_neg1, -1));

    PwValue int_2 = PwSigned(2);
    TEST(pw_is_signed(&int_2));
    TEST(pw_equal(&int_2, 2));
    PwValue int_3 = PwUnsigned(3);
    TEST(pw_is_unsigned(&int_3));
    TEST(pw_equal(&int_3, 3));

    // Float values
    PwValue f_0 = PwFloat(0.0);
    PwValue f_1 = PwFloat(1.0);
    PwValue f_neg1 = PwFloat(-1.0);
    TEST(pw_is_float(&f_0));
    TEST(pw_is_float(&f_1));
    TEST(pw_is_float(&f_neg1));
    TEST(pw_equal(&f_0, &f_0));
    TEST(pw_equal(&f_1, &f_1));
    TEST(pw_equal(&f_0, 0.0));
    TEST(!pw_equal(&f_0, 1.0));
    TEST(pw_equal(&f_1, 1.0));
    TEST(pw_equal(&f_neg1, -1.0));
    TEST(!pw_equal(&f_neg1, 1.0));

    PwValue f_2 = PwFloat(2.0);
    TEST(pw_is_float(&f_2));
    TEST(pw_equal(&f_2, 2.0));
    PwValue f_3 = PwFloat(3.0f);
    TEST(pw_is_float(&f_3));
    TEST(pw_equal(&f_3, 3.0));
    TEST(pw_equal(&f_3, 3.0f));

    // null vs null
    TEST(pw_equal(&null_1, nullptr));

    // null vs bool
    TEST(!pw_equal(&null_1, &bool_true));
    TEST(!pw_equal(&null_1, &bool_false));
    TEST(!pw_equal(&null_1, true));
    TEST(!pw_equal(&null_1, false));

    // null vs int
    TEST(!pw_equal(&null_1, &int_0));
    TEST(!pw_equal(&null_1, &int_1));
    TEST(!pw_equal(&null_1, &int_neg1));
    TEST(!pw_equal(&null_1, (char) 2));
    TEST(!pw_equal(&null_1, (unsigned char) 2));
    TEST(!pw_equal(&null_1, (short) 2));
    TEST(!pw_equal(&null_1, (unsigned short) 2));
    TEST(!pw_equal(&null_1, 2));
    TEST(!pw_equal(&null_1, 2U));
    TEST(!pw_equal(&null_1, (unsigned long) 2));
    TEST(!pw_equal(&null_1, (unsigned long long) 2));

    // null vs float
    TEST(!pw_equal(&null_1, &f_0));
    TEST(!pw_equal(&null_1, &f_1));
    TEST(!pw_equal(&null_1, &f_neg1));
    TEST(!pw_equal(&null_1, 2.0f));
    TEST(!pw_equal(&null_1, 2.0));

    // bool vs null
    TEST(!pw_equal(&bool_true, &null_1));
    TEST(!pw_equal(&bool_false, &null_1));
    TEST(!pw_equal(&bool_true, nullptr));
    TEST(!pw_equal(&bool_false, nullptr));

    // bool vs bool
    TEST(pw_equal(&bool_true, true));
    TEST(!pw_equal(&bool_true, false));
    TEST(pw_equal(&bool_false, false));
    TEST(!pw_equal(&bool_false, true));

    TEST(pw_equal(&bool_true, &bool_true));
    TEST(pw_equal(&bool_false, &bool_false));
    TEST(!pw_equal(&bool_true, &bool_false));
    TEST(!pw_equal(&bool_false, &bool_true));

    // bool vs int
    TEST(!pw_equal(&bool_true, &int_0));
    TEST(!pw_equal(&bool_true, &int_1));
    TEST(!pw_equal(&bool_true, &int_neg1));
    TEST(!pw_equal(&bool_false, &int_0));
    TEST(!pw_equal(&bool_false, &int_1));
    TEST(!pw_equal(&bool_false, &int_neg1));
    TEST(!pw_equal(&bool_true, (char) 0));
    TEST(!pw_equal(&bool_true, (char) 2));
    TEST(!pw_equal(&bool_false, (char) 0));
    TEST(!pw_equal(&bool_false, (char) 2));
    TEST(!pw_equal(&bool_true, (unsigned char) 0));
    TEST(!pw_equal(&bool_true, (unsigned char) 2));
    TEST(!pw_equal(&bool_false, (unsigned char) 0));
    TEST(!pw_equal(&bool_false, (unsigned char) 2));
    TEST(!pw_equal(&bool_true, (short) 0));
    TEST(!pw_equal(&bool_true, (short) 2));
    TEST(!pw_equal(&bool_false, (short) 0));
    TEST(!pw_equal(&bool_false, (short) 2));
    TEST(!pw_equal(&bool_true, (unsigned short) 0));
    TEST(!pw_equal(&bool_true, (unsigned short) 2));
    TEST(!pw_equal(&bool_false, (unsigned short) 0));
    TEST(!pw_equal(&bool_false, (unsigned short) 2));
    TEST(!pw_equal(&bool_true, 0));
    TEST(!pw_equal(&bool_true, 2));
    TEST(!pw_equal(&bool_false, 0));
    TEST(!pw_equal(&bool_false, 2));
    TEST(!pw_equal(&bool_true, 0U));
    TEST(!pw_equal(&bool_true, 2U));
    TEST(!pw_equal(&bool_false, 0U));
    TEST(!pw_equal(&bool_false, 2U));
    TEST(!pw_equal(&bool_true, 0L));
    TEST(!pw_equal(&bool_true, 2L));
    TEST(!pw_equal(&bool_false, 0L));
    TEST(!pw_equal(&bool_false, 2L));
    TEST(!pw_equal(&bool_true, 0UL));
    TEST(!pw_equal(&bool_true, 2UL));
    TEST(!pw_equal(&bool_false, 0UL));
    TEST(!pw_equal(&bool_false, 2UL));
    TEST(!pw_equal(&bool_true, 0LL));
    TEST(!pw_equal(&bool_true, 2LL));
    TEST(!pw_equal(&bool_false, 0LL));
    TEST(!pw_equal(&bool_false, 2LL));
    TEST(!pw_equal(&bool_true, 0ULL));
    TEST(!pw_equal(&bool_true, 2ULL));
    TEST(!pw_equal(&bool_false, 0ULL));
    TEST(!pw_equal(&bool_false, 2ULL));

    // bool vs float
    TEST(!pw_equal(&bool_true, &f_0));
    TEST(!pw_equal(&bool_true, &f_1));
    TEST(!pw_equal(&bool_true, &f_neg1));
    TEST(!pw_equal(&bool_false, &f_0));
    TEST(!pw_equal(&bool_false, &f_1));
    TEST(!pw_equal(&bool_false, &f_neg1));
    TEST(!pw_equal(&bool_true, 0.0f));
    TEST(!pw_equal(&bool_true, 2.0f));
    TEST(!pw_equal(&bool_false, 0.0f));
    TEST(!pw_equal(&bool_false, 2.0f));
    TEST(!pw_equal(&bool_true, 0.0f));
    TEST(!pw_equal(&bool_true, 2.0f));
    TEST(!pw_equal(&bool_false, 0.0f));
    TEST(!pw_equal(&bool_false, 2.0f));
    TEST(!pw_equal(&bool_true, 0.0));
    TEST(!pw_equal(&bool_true, 2.0));
    TEST(!pw_equal(&bool_false, 0.0));
    TEST(!pw_equal(&bool_false, 2.0));
    TEST(!pw_equal(&bool_true, 0.0));
    TEST(!pw_equal(&bool_true, 2.0));
    TEST(!pw_equal(&bool_false, 0.0));
    TEST(!pw_equal(&bool_false, 2.0));

    // int vs null
    TEST(!pw_equal(&int_0, &null_1));
    TEST(!pw_equal(&int_0, nullptr));
    TEST(!pw_equal(&int_1, &null_1));
    TEST(!pw_equal(&int_neg1, &null_1));

    // int vs bool
    TEST(!pw_equal(&int_0, &bool_true));
    TEST(!pw_equal(&int_0, &bool_false));
    TEST(!pw_equal(&int_0, true));
    TEST(!pw_equal(&int_0, false));
    TEST(!pw_equal(&int_1, &bool_true));
    TEST(!pw_equal(&int_1, &bool_false));
    TEST(!pw_equal(&int_1, true));
    TEST(!pw_equal(&int_1, false));
    TEST(!pw_equal(&int_neg1, &bool_true));
    TEST(!pw_equal(&int_neg1, &bool_false));
    TEST(!pw_equal(&int_neg1, true));
    TEST(!pw_equal(&int_neg1, false));

    // int vs int
    TEST(pw_equal(&int_0, &int_0));
    TEST(pw_equal(&int_1, &int_1));
    TEST(pw_equal(&int_neg1, &int_neg1));
    TEST(!pw_equal(&int_0, &int_1));
    TEST(!pw_equal(&int_1, &int_0));
    TEST(!pw_equal(&int_neg1, &int_0));
    TEST(!pw_equal(&int_0, &int_neg1));
    TEST(pw_equal(&int_1, (char) 1));
    TEST(!pw_equal(&int_1, (char) 2));
    TEST(!pw_equal(&int_1, (char) -1));
    TEST(pw_equal(&int_1, (unsigned char) 1));
    TEST(!pw_equal(&int_1, (unsigned char) 2));
    TEST(!pw_equal(&int_1, (unsigned char) 0));
    TEST(pw_equal(&int_1, (short) 1));
    TEST(!pw_equal(&int_1, (short) 2));
    TEST(!pw_equal(&int_1, (short) -1));
    TEST(pw_equal(&int_1, (unsigned short) 1));
    TEST(!pw_equal(&int_1, (unsigned short) 2));
    TEST(!pw_equal(&int_1, (unsigned short) 0));
    TEST(pw_equal(&int_1, 1));
    TEST(!pw_equal(&int_1, 2));
    TEST(!pw_equal(&int_1, -1));
    TEST(pw_equal(&int_1, 1U));
    TEST(!pw_equal(&int_1, 2U));
    TEST(!pw_equal(&int_1, 0U));
    TEST(pw_equal(&int_1, 1L));
    TEST(!pw_equal(&int_1, 2L));
    TEST(!pw_equal(&int_1, -1L));
    TEST(pw_equal(&int_1, 1UL));
    TEST(!pw_equal(&int_1, 2UL));
    TEST(!pw_equal(&int_1, 0UL));
    TEST(pw_equal(&int_1, 1LL));
    TEST(!pw_equal(&int_1, 2LL));
    TEST(!pw_equal(&int_1, -1LL));
    TEST(pw_equal(&int_1, 1ULL));
    TEST(!pw_equal(&int_1, 2ULL));
    TEST(!pw_equal(&int_1, 0ULL));

    // int vs float
    TEST(pw_equal(&int_0, &f_0));
    TEST(pw_equal(&int_1, &f_1));
    TEST(pw_equal(&int_neg1, &f_neg1));
    TEST(!pw_equal(&int_0, &f_1));
    TEST(!pw_equal(&int_1, &f_0));
    TEST(!pw_equal(&int_neg1, &f_0));
    TEST(!pw_equal(&int_0, &f_neg1));
    TEST(pw_equal(&int_1, 1.0));
    TEST(!pw_equal(&int_1, 2.0));
    TEST(!pw_equal(&int_1, -1.0));
    TEST(pw_equal(&int_1, 1.0f));
    TEST(!pw_equal(&int_1, 2.0f));
    TEST(!pw_equal(&int_1, -1.0f));

    // float vs null
    TEST(!pw_equal(&f_0, &null_1));
    TEST(!pw_equal(&f_0, nullptr));
    TEST(!pw_equal(&f_1, &null_1));
    TEST(!pw_equal(&f_neg1, &null_1));

    // float vs bool
    TEST(!pw_equal(&f_0, &bool_true));
    TEST(!pw_equal(&f_0, &bool_false));
    TEST(!pw_equal(&f_0, true));
    TEST(!pw_equal(&f_0, false));
    TEST(!pw_equal(&f_1, &bool_true));
    TEST(!pw_equal(&f_1, &bool_false));
    TEST(!pw_equal(&f_1, true));
    TEST(!pw_equal(&f_1, false));
    TEST(!pw_equal(&f_neg1, &bool_true));
    TEST(!pw_equal(&f_neg1, &bool_false));
    TEST(!pw_equal(&f_neg1, true));
    TEST(!pw_equal(&f_neg1, false));

    // float vs int
    TEST(pw_equal(&f_0, &int_0));
    TEST(pw_equal(&f_1, &int_1));
    TEST(pw_equal(&f_neg1, &int_neg1));
    TEST(!pw_equal(&f_0, &int_1));
    TEST(!pw_equal(&f_1, &int_0));
    TEST(!pw_equal(&f_neg1, &int_0));
    TEST(!pw_equal(&f_0, &int_neg1));
    TEST(pw_equal(&f_1, (char) 1));
    TEST(!pw_equal(&f_1, (char) 2));
    TEST(!pw_equal(&f_1, (char) -1));
    TEST(pw_equal(&f_1, (unsigned char) 1));
    TEST(!pw_equal(&f_1, (unsigned char) 2));
    TEST(!pw_equal(&f_1, (unsigned char) 0));
    TEST(pw_equal(&f_1, (short) 1));
    TEST(!pw_equal(&f_1, (short) 2));
    TEST(!pw_equal(&f_1, (short) -1));
    TEST(pw_equal(&f_1, (unsigned short) 1));
    TEST(!pw_equal(&f_1, (unsigned short) 2));
    TEST(!pw_equal(&f_1, (unsigned short) 0));
    TEST(pw_equal(&f_1, 1));
    TEST(!pw_equal(&f_1, 2));
    TEST(!pw_equal(&f_1, -1));
    TEST(pw_equal(&f_1, 1U));
    TEST(!pw_equal(&f_1, 2U));
    TEST(!pw_equal(&f_1, 0U));
    TEST(pw_equal(&f_1, 1L));
    TEST(!pw_equal(&f_1, 2L));
    TEST(!pw_equal(&f_1, -1L));
    TEST(pw_equal(&f_1, 1UL));
    TEST(!pw_equal(&f_1, 2UL));
    TEST(!pw_equal(&f_1, 0UL));
    TEST(pw_equal(&f_1, 1LL));
    TEST(!pw_equal(&f_1, 2LL));
    TEST(!pw_equal(&f_1, -1LL));
    TEST(pw_equal(&f_1, 1ULL));
    TEST(!pw_equal(&f_1, 2ULL));
    TEST(!pw_equal(&f_1, 0ULL));

    // float vs float
    TEST(!pw_equal(&f_0, &f_1));
    TEST(!pw_equal(&f_1, &f_0));
    TEST(!pw_equal(&f_neg1, &f_0));
    TEST(!pw_equal(&f_0, &f_neg1));
    TEST(pw_equal(&f_1, 1.0));
    TEST(!pw_equal(&f_1, 2.0));
    TEST(!pw_equal(&f_1, -1.0));
    TEST(pw_equal(&f_1, 1.0f));
    TEST(!pw_equal(&f_1, 2.0f));
    TEST(!pw_equal(&f_1, -1.0f));
}

void test_string()
{
    // zero is not space
    TEST(pw_isspace(0) == false);

    { // testing char_size=1
        PwValue v = PW_NULL;
        if (!pw_create_empty_string(0, 1, &v)) {
            panic();
        }
        TEST(pw_strlen(&v) == 0);
        TEST(_pw_string_capacity(&v) == 12);
        TEST(v.char_size == 1);
        //pw_dump(stderr, &v);

        if (!pw_string_append(&v, "hello")) {
            panic();
        }
        TEST(pw_strlen(&v) == 5);
        TEST(_pw_string_capacity(&v) == 12);
        //pw_dump(stderr, &v);

        if (!pw_string_append(&v, '!')) {
            panic();
        }

        TEST(pw_strlen(&v) == 6);
        TEST(_pw_string_capacity(&v) == 12);
        //pw_dump(stderr, &v);

        // XXX TODO increase capacity to more than 64K
        for (int i = 0; i < 250; i++) {
            if (!pw_string_append(&v, ' ')) {
                panic();
            }
        }
        TEST(pw_strlen(&v) == 256);
        TEST(v.char_size == 1);
        //pw_dump(stderr, &v);

        if (!pw_string_append(&v, "pet")) {
            panic();
        }
        if (!pw_string_erase(&v, 5, 255)) {
            panic();
        }
        TEST(pw_equal(&v, "hello pet"));
        //pw_dump(stderr, &v);
        TEST(!pw_equal(&v, ""));

        // test comparison
        PwValue v2 = PW_NULL;
        if (!pw_create_string("hello pet", &v2)) {
            panic();
        }
        TEST(pw_equal(&v, &v2));
        TEST(pw_equal(&v2, "hello pet"));
        TEST(!pw_equal(&v, "hello Pet"));
        TEST(!pw_equal(&v2, "hello Pet"));
        TEST(pw_equal(&v, u8"hello pet"));
        TEST(pw_equal(&v2, u8"hello pet"));
        TEST(!pw_equal(&v, u8"hello Pet"));
        TEST(!pw_equal(&v2, u8"hello Pet"));
        TEST(pw_equal(&v, U"hello pet"));
        TEST(pw_equal(&v2, U"hello pet"));
        TEST(!pw_equal(&v, U"hello Pet"));
        TEST(!pw_equal(&v2, U"hello Pet"));

        PwValue v3 = PW_NULL;
        if (!pw_create_string("hello pet", &v3)) {
            panic();
        }
        // test C string
        PW_CSTRING_LOCAL(cv3, &v3);
        TEST(strcmp(cv3, "hello pet") == 0);

        // test substring
        TEST(pw_substring_eq(&v, 4, 7, "o p"));
        TEST(!pw_substring_eq(&v, 4, 7, ""));
        TEST(pw_substring_eq(&v, 0, 4, "hell"));
        TEST(pw_substring_eq(&v, 7, 100, "et"));
        TEST(pw_substring_eq(&v, 4, 7, u8"o p"));
        TEST(!pw_substring_eq(&v, 4, 7, u8""));
        TEST(pw_substring_eq(&v, 0, 4, u8"hell"));
        TEST(pw_substring_eq(&v, 7, 100, u8"et"));
        TEST(pw_substring_eq(&v, 4, 7, U"o p"));
        TEST(!pw_substring_eq(&v, 4, 7, U""));
        TEST(pw_substring_eq(&v, 0, 4, U"hell"));
        TEST(pw_substring_eq(&v, 7, 100, U"et"));

        // test erase and truncate
        if (!pw_string_erase(&v, 4, 255)) {
            panic();
        }
        TEST(pw_equal(&v, "hell"));

        if (!pw_string_erase(&v, 0, 2)) {
            panic();
        }
        TEST(pw_equal(&v, "ll"));

        if (!pw_string_truncate(&v, 0)) {
            panic();
        }
        TEST(pw_equal(&v, ""));

        TEST(pw_strlen(&v) == 0);
        TEST(_pw_string_capacity(&v) == 264);
        //pw_dump(stderr, &v);

        // test append substring
        if (!pw_string_append_substring(&v, "0123456789", 3, 7)) {
            panic();
        }
        TEST(pw_equal(&v, "3456"));
        if (!pw_string_append_substring(&v, u8"0123456789", 3, 7)) {
            panic();
        }
        TEST(pw_equal(&v, "34563456"));
        if (!pw_string_append_substring(&v, U"0123456789", 3, 7)) {
            panic();
        }
        TEST(pw_equal(&v, "345634563456"));
        if (!pw_string_truncate(&v, 0)) {
            panic();
        }

        // change char size to 2-byte by appending wider chars -- the string will be copied
        if (!pw_string_append(&v, u8"สวัสดี")) {
            panic();
        }
        TEST(pw_strlen(&v) == 6);
        TEST(_pw_string_capacity(&v) == 268);  // capacity is slightly changed because of alignment and char_size increase
        TEST(v.char_size == 2);
        TEST(pw_equal(&v, u8"สวัสดี"));
        //pw_dump(stderr, &v);
    }

    { // testing char_size=2
        PwValue v = PW_NULL;
        if (!pw_create_empty_string(1, 2, &v)) {
            panic();
        }
        TEST(pw_strlen(&v) == 0);
        TEST(_pw_string_capacity(&v) == 6);
        TEST(v.char_size == 2);
        //pw_dump(stderr, &v);

        if (!pw_string_append(&v, u8"สบาย")) {
            panic();
        }
        TEST(pw_strlen(&v) == 4);
        TEST(_pw_string_capacity(&v) == 6);
        //pw_dump(stderr, &v);

        if (!pw_string_append(&v, 0x0e14)) {
            panic();
        }
        if (!pw_string_append(&v, 0x0e35)) {
            panic();
        }
        TEST(pw_strlen(&v) == 6);
        TEST(_pw_string_capacity(&v) == 6);
        TEST(pw_equal(&v, u8"สบายดี"));
        //pw_dump(stderr, &v);

        // test truncate
        if (!pw_string_truncate(&v, 4)) {
            panic();
        }
        TEST(pw_equal(&v, u8"สบาย"));
        TEST(!pw_equal(&v, ""));
        //pw_dump(stderr, &v);

        // increase capacity to 2 bytes
        for (int i = 0; i < 251; i++) {
            if (!pw_string_append(&v, ' ')) {
                panic();
            }
        }
        TEST(pw_strlen(&v) == 255);
        TEST(_pw_string_capacity(&v) == 260);
        TEST(v.char_size == 2);
        //pw_dump(stderr, &v);

        if (!pw_string_append(&v, U"สบาย")) {
            panic();
        }
        if (!pw_string_erase(&v, 4, 255)) {
            panic();
        }
        TEST(pw_equal(&v, u8"สบายสบาย"));
        TEST(!pw_equal(&v, ""));

        // test comparison
        PwValue v2 = PW_NULL;
        if (!pw_create_string(u8"สบายสบาย", &v2)) {
            panic();
        }
        TEST(pw_equal(&v, &v2));
        TEST(pw_equal(&v, u8"สบายสบาย"));
        TEST(pw_equal(&v2, u8"สบายสบาย"));
        TEST(!pw_equal(&v, u8"ความสบาย"));
        TEST(!pw_equal(&v2, u8"ความสบาย"));
        TEST(pw_equal(&v, U"สบายสบาย"));
        TEST(pw_equal(&v2, U"สบายสบาย"));
        TEST(!pw_equal(&v, U"ความสบาย"));
        TEST(!pw_equal(&v2, U"ความสบาย"));

        // test substring
        TEST(pw_substring_eq(&v, 3, 5, u8"ยส"));
        TEST(!pw_substring_eq(&v, 3, 5, u8""));
        TEST(pw_substring_eq(&v, 0, 3, u8"สบา"));
        TEST(pw_substring_eq(&v, 6, 100, u8"าย"));
        TEST(pw_substring_eq(&v, 3, 5, U"ยส"));
        TEST(!pw_substring_eq(&v, 3, 5, U""));
        TEST(pw_substring_eq(&v, 0, 3, U"สบา"));
        TEST(pw_substring_eq(&v, 6, 100, U"าย"));

        // test erase and truncate
        if (!pw_string_erase(&v, 4, 255)) {
            panic();
        }
        TEST(pw_equal(&v, u8"สบาย"));

        if (!pw_string_erase(&v2, 0, 4)) {
            panic();
        }
        TEST(pw_equal(&v, u8"สบาย"));

        if (!pw_string_truncate(&v, 0)) {
            panic();
        }
        TEST(pw_equal(&v, ""));

        // test append substring
        if (!pw_string_append_substring(&v, u8"สบายสบาย", 1, 4)) {
            panic();
        }
        TEST(pw_equal(&v, u8"บาย"));
        if (!pw_string_append_substring(&v, U"สบายสบาย", 1, 4)) {
            panic();
        }
        TEST(pw_equal(&v, U"บายบาย"));
        if (!pw_string_truncate(&v, 0)) {
            panic();
        }
        TEST(pw_strlen(&v) == 0);
        TEST(_pw_string_capacity(&v) == 260);
        //pw_dump(stderr, &v);
    }

    { // testing char_size=3
        PwValue v = PW_NULL;
        if (!pw_create_empty_string(1, 3, &v)) {
            panic();
        }
        TEST(pw_strlen(&v) == 0);
        TEST(_pw_string_capacity(&v) == 4);
        TEST(v.char_size == 3);
        //pw_dump(stderr, &v);
    }

    { // testing char_size=4
        PwValue v = PW_NULL;
        if (!pw_create_empty_string(1, 4, &v)) {
            panic();
        }
        TEST(pw_strlen(&v) == 0);
        TEST(_pw_string_capacity(&v) == 3);
        TEST(v.char_size == 4);
        //pw_dump(stderr, &v);
    }

    { // test trimming
        PwValue v = PW_NULL;
        if (!pw_create_string(u8"  สวัสดี   ", &v)) {
            panic();
        }
        TEST(pw_strlen(&v) == 11);
        if (!pw_string_ltrim(&v)) {
            panic();
        }
        TEST(pw_equal(&v, u8"สวัสดี   "));
        if (!pw_string_rtrim(&v)) {
            panic();
        }
        TEST(pw_equal(&v, u8"สวัสดี"));
        TEST(pw_strlen(&v) == 6);
    }

    { // test pw_strcat (by value)
        PwValue v = PW_NULL;
        if (!pw_strcat(&v, PwStaticString("Hello! "), PwString("Thanks"),
                       PwStringUtf32(U"🙏"), PwStringUtf8(u8"สวัสดี"))) {
            panic();
        }
        TEST(pw_equal(&v, U"Hello! Thanks🙏สวัสดี"));
        //pw_dump(stderr, &v);
    }

    { // test split/join
        PwValue str = PW_NULL;
        if (!pw_create_string(U"สบาย/สบาย/yo/yo", &str)) {
            panic();
        }
        PwValue array = PW_NULL;
        if (!pw_string_split_chr(&str, '/', 0, &array)) {
            panic();
        }
        //pw_dump(stderr, &array);
        PwValue array2 = PW_NULL;
        if (!pw_string_rsplit_chr(&str, '/', 1, &array2)) {
            panic();
        }
        //pw_dump(stderr, &array2);
        PwValue first = PW_NULL;
        if (!pw_array_item(&array2, 0, &first)) {
            panic();
        }
        PwValue last = PW_NULL;
        if (!pw_array_item(&array2, 1, &last)) {
            panic();
        }
        TEST(pw_equal(&first, U"สบาย/สบาย/yo"));
        TEST(pw_equal(&last, "yo"));
        PwValue array3 = PW_NULL;
        if (!pw_string_split_chr(&str, '/', 1, &array3)) {
            panic();
        }
        //pw_dump(stderr, &array3);
        PwValue first3 = PW_NULL;
        if (!pw_array_item(&array3, 0, &first3)) {
            panic();
        }
        PwValue last3 = PW_NULL;
        if (!pw_array_item(&array3, 1, &last3)) {
            panic();
        }
        TEST(pw_equal(&first3, U"สบาย"));
        TEST(pw_equal(&last3, U"สบาย/yo/yo"));
        PwValue v = PW_NULL;
        if (!pw_array_join('/', &array, &v)) {
            panic();
        }
        TEST(pw_equal(&v, U"สบาย/สบาย/yo/yo"));
    }

    // test append_buffer
    {
        char8_t data[2500];
        memset(data, '1', sizeof(data));

        PwValue str = PW_STRING("");

        if (!pw_string_append_buffer(&str, data, sizeof(data))) {
            panic();
        }
        TEST(_pw_string_capacity(&str) >= pw_strlen(&str));
        TEST(pw_strlen(&str) == 2500);
        //pw_dump(stderr, &str);
    }

    { // test startswith/endswith
        PwValue str = PW_NULL;
        if (!pw_create_string("hello world", &str)) {
            panic();
        }
        TEST(pw_startswith(&str, 'h'));
        TEST(!pw_startswith(&str, U'ค'));
        TEST(pw_startswith(&str, "hello"));
        TEST(!pw_startswith(&str, "world"));
        TEST(pw_endswith(&str, 'd'));
        TEST(!pw_endswith(&str, U'า'));
        TEST(pw_endswith(&str, "world"));
        TEST(!pw_endswith(&str, "hello"));
    }
    {
        PwValue str = PW_NULL;
        if (!pw_create_string(u8"ความคืบหน้า", &str)) {
            panic();
        }
        TEST(!pw_startswith(&str, 'h'));
        TEST(pw_startswith(&str, U'ค'));
        TEST(pw_startswith(&str, u8"ความ"));
        TEST(pw_startswith(&str, U"ความ"));
        TEST(!pw_startswith(&str, "wow"));
        TEST(!pw_endswith(&str, 'd'));
        TEST(pw_endswith(&str, U'า'));
        TEST(pw_endswith(&str, u8"คืบหน้า"));
        TEST(pw_endswith(&str, U"คืบหน้า"));
        TEST(!pw_endswith(&str, "wow"));
    }

    { // test isdigit, is_ascii_digit
        PwValue empty = PW_STRING("");
        PwValue nondigit = PW_NULL;
        if (!pw_create_string(u8"123รูปโป๊", &nondigit)) {
            panic();
        }
        PwValue digit = PW_NULL;
        if (!pw_create_string("456", &digit)) {
            panic();
        }
        TEST(!pw_string_is_ascii_digit(&empty));
        TEST(!pw_string_is_ascii_digit(&nondigit));
        TEST(pw_string_is_ascii_digit(&digit));

        PwValue th_digit = PW_NULL;
        if (!pw_create_string(u8"๑๒๓๔๕", &th_digit)) {
            panic();
        }
        TEST(pw_string_isdigit(&th_digit));

        // fail:
        // TEST(pw_isdigit(U'零'));
        // TEST(pw_isdigit(U'〇'));
        //PwValue zh_digit = PW_NULL;
        //if (!pw_create_string(u8"一二三四五", &zh_digit)) {
        //    panic();
        //}
        //TEST(pw_string_isdigit(&zh_digit));
    }
    { // test isalnum
        TEST(pw_isalnum(U'๕'));
    }
    { // test isspace, isblank
        TEST(pw_isspace(' '));
        TEST(pw_isblank(' '));
        TEST(pw_isspace('\t'));
        TEST(pw_isblank('\t'));
        TEST(pw_isspace('\r'));
        TEST(!pw_isblank('\r'));
        TEST(pw_isspace('\n'));
        TEST(!pw_isblank('\n'));
        // nbsp
        TEST(pw_isspace(160));
        TEST(pw_isblank(160));
        // En Quad
        TEST(pw_isspace(8192));
        TEST(pw_isblank(8192));
        // Em Quad
        TEST(pw_isspace(8193));
        TEST(pw_isblank(8193));
        // En Space
        TEST(pw_isspace(8194));
        TEST(pw_isblank(8194));
        // Em Space
        TEST(pw_isspace(8195));
        TEST(pw_isblank(8195));
        // Three-Per-Em Space
        TEST(pw_isspace(8196));
        TEST(pw_isblank(8196));
        // Four-Per-Em Space
        TEST(pw_isspace(8197));
        TEST(pw_isblank(8197));
        // Six-Per-Em Space
        TEST(pw_isspace(8198));
        TEST(pw_isblank(8198));
        // Figure Space
        TEST(pw_isspace(8199));
        TEST(pw_isblank(8199));
        // Punctuation Space
        TEST(pw_isspace(8200));
        TEST(pw_isblank(8200));
        // Thin Space
        TEST(pw_isspace(8201));
        TEST(pw_isblank(8201));
        // Hair Space
        TEST(pw_isspace(8202));
        TEST(pw_isblank(8202));
        // Line Separator
        TEST(pw_isspace(8232));
        TEST(!pw_isblank(8232));
        // Medium Mathematical Space
        TEST(pw_isspace(8287));
        TEST(pw_isblank(8287));
        // Ideographic Space
        TEST(pw_isspace(12288));
        TEST(pw_isblank(12288));
    }
}

void test_array()
{
    PwValue array = PW_NULL;
    if (!pw_create_array(&array)) {
        panic();
    }

    TEST(pw_array_length(&array) == 0);

    for(unsigned i = 0; i < 1000; i++) {
        {
            PwValue item = PwUnsigned(i);
            if (!pw_array_append(&array, &item)) {
                panic();
            }
        }
        TEST(pw_array_length(&array) == i + 1);
        {
            PwValue v = PW_NULL;
            if (!pw_array_item(&array, i, &v)) {
                panic();
            }
            PwValue item = PW_UNSIGNED(i);
            TEST(pw_equal(&v, &item));
        }
    }
    {
        PwValue item = PW_NULL;
        if (!pw_array_item(&array, -2, &item)) {
            panic();
        }
        PwValue v = PW_UNSIGNED(998);
        TEST(pw_equal(&v, &item));
    }

    pw_array_del(&array, 100, 200);
    TEST(pw_array_length(&array) == 900);

    {
        PwValue item = PW_NULL;
        if (!pw_array_item(&array, 99, &item)) {
            panic();
        }
        PwValue v = PW_UNSIGNED(99);
        TEST(pw_equal(&v, &item));
    }
    {
        PwValue item = PW_NULL;
        if (!pw_array_item(&array, 100, &item)) {
            panic();
        }
        PwValue v = PwUnsigned(200);
        TEST(pw_equal(&v, &item));
    }
    {
        PwValue slice = PW_NULL;
        if (!pw_array_slice(&array, 750, 850, &slice)) {
            panic();
        }
        TEST(pw_array_length(&slice) == 100);
        {
            PwValue item = PW_NULL;
            if (!pw_array_item(&slice, 1, &item)) {
                panic();
            }
            TEST(pw_equal(&item, 851));
        }
        {
            PwValue item = PW_NULL;
            if (!pw_array_item(&slice, 98, &item)) {
                panic();
            }
            TEST(pw_equal(&item, 948));
        }
    }

    PwValue pulled = PW_NULL;
    if (!pw_array_pull(&array, &pulled)) {
        panic();
    }
    TEST(pw_equal(&pulled, 0));
    TEST(pw_array_length(&array) == 899);
    if (!pw_array_pull(&array, &pulled)) {
        panic();
    }
    TEST(pw_equal(&pulled, 1));
    TEST(pw_array_length(&array) == 898);

    { // test join
        PwValue array = PW_NULL;
        if (!pw_create_array(&array)) {
            panic();
        }
        if (!pw_array_append(&array, "Hello")) {
            panic();
        }
        if (!pw_array_append(&array, u8"สวัสดี")) {
            panic();
        }
        if (!pw_array_append(&array, "Thanks")) {
            panic();
        }
        if (!pw_array_append(&array, U"mulțumesc")) {
            panic();
        }
        PwValue v = PW_NULL;
        if (!pw_array_join('/', &array, &v)) {
            panic();
        }
        TEST(pw_equal(&v, U"Hello/สวัสดี/Thanks/mulțumesc"));
        //pw_dump(stderr, &v);
    }
    { // test join with Ascii/UTF8/UTF32
        PwValue array = PW_NULL;
        if (!pw_create_array(&array)) {
            panic();
        }
        PwValue sawatdee   = PwStringUtf8(u8"สวัสดี");
        PwValue thanks     = PW_STRING("Thanks");
        PwValue multsumesc = PW_STATIC_STRING_UTF32(U"mulțumesc");
        PwValue wat        = PW_STRING_UTF32(U"🙏");
        if (!pw_array_append(&array, "Hello")) {
            panic();
        }
        if (!pw_array_append(&array, &sawatdee)) {
            panic();
        }
        if (!pw_array_append(&array, &thanks)) {
            panic();
        }
        if (!pw_array_append(&array, &multsumesc)) {
            panic();
        }
        PwValue v = PW_NULL;
        if (!pw_array_join(&wat, &array, &v)) {
            panic();
        }
        TEST(pw_equal(&v, U"Hello🙏สวัสดี🙏Thanks🙏mulțumesc"));
        //pw_dump(stderr, &v);
    }
    { // test dedent
        PwValue array = PW_NULL;
        if (!pw_array_va(&array,
                      PwStaticString("   first line"),
                      PwStaticString("  second line"),
                      PwStaticString("    third line")
                     )) {
            panic();
        }
        if (!pw_array_dedent(&array)) {
            panic();
        }
        //pw_dump(stderr, &array);
        PwValue v = PW_NULL;
        if (!pw_array_join(',', &array, &v)) {
            panic();
        }
        TEST(pw_equal(&v, " first line,second line,  third line"));
        //pw_dump(stderr, &v);
    }
}

void test_map()
{
    {
        PwValue map = PW_NULL;
        if (!pw_create_map(&map)) {
            panic();
        }
        PwValue key = PwUnsigned(0);
        PwValue value = PwBool(false);

        if (!pw_map_update(&map, &key, &value)) {
            panic();
        }
        TEST(pw_map_length(&map) == 1);

        key = PwUnsigned(0);
        TEST(pw_map_has_key(&map, &key));

        key = PwNull();
        TEST(!pw_map_has_key(&map, &key));

        for (int i = 1; i < 50; i++) {
            key = PwUnsigned(i);
            value = PwUnsigned(i);
            if (!pw_map_update(&map, &key, &value)) {
                panic();
            }
        }
        if (!pw_map_del(&map, 25)) {
            panic();
        }
        TEST(pw_map_length(&map) == 49);
        //pw_dump(stderr, &map);
    }
    {
        PwValue map = PW_NULL;
        if (!pw_map_va(&map,
            PwString("let's"),        PwString("go!"),
            PwNull(),                 PwBool(true),
            PwBool(true),             PwString("true"),
            PwSigned(-10),            PwBool(false),
            PwSigned('b'),            PwSigned(-42),
            PwUnsigned(100),          PwSigned(-1000000L),
            PwUnsigned(300000000ULL), PwFloat(1.23),
            PwStringUtf8(u8"สวัสดี"),   PwStaticString(U"สบาย"),
            PwString("finally"),      pwva_map(PwString("ok"), PwString("done"))
        )) {
            panic();
        }
        TEST(pw_map_length(&map) == 9);
        //pw_dump(stderr, &map);
    }
}

void test_file()
{
    // UTF-8 crossing read boundary
    {
        char8_t a[] = u8"###################################################################################################\n";
        char8_t b[] = u8"##############################################################################################\n";
        char8_t c[] = u8"สบาย\n";

        char8_t data_filename[] = u8"./test/data/utf8-crossing-buffer-boundary";

        PwValue file = PW_NULL;
        if (!pw_file_open(data_filename, O_RDONLY, 0, &file)) {
            panic();
        }
        if (!pw_start_read_lines(&file)) {
            panic();
        }
        PwValue line = PW_STRING("");
        for (;;) {{
            if (!pw_read_line_inplace(&file, &line)) {
                panic();
            }
            if (!pw_equal(&line, a)) {
                break;
            }
        }}
        TEST(pw_equal(&line, b));
        if (!pw_read_line_inplace(&file, &line)) {
            panic();
        }
        TEST(pw_equal(&line, c));

        { // test path functions
            PwValue s = PW_NULL;
            if (!pw_create_string("/bin/bash", &s)) {
                panic();
            }
            PwValue basename = PW_NULL;
            if (!pw_basename(&s, &basename)) {
                panic();
            }
            //pw_dump(stderr, &basename);
            TEST(pw_equal(&basename, "bash"));
            PwValue dirname = PW_NULL;
            if (!pw_dirname(&s, &dirname)) {
                panic();
            }
            //pw_dump(stderr, &dirname);
            TEST(pw_equal(&dirname, "/bin"));
            PwValue path = PW_NULL;
            if (!pw_path(&path, PwString(""), PwString("bin"), PwString("bash"))) {
                panic();
            }
            TEST(pw_equal(&path, "/bin/bash"));
            //pw_dump(stderr, &path);

            PwValue s2 = PW_NULL;
            if (!pw_create_string("blahblahblah", &s2)) {
                panic();
            }
            //pw_dump(stderr, &s2);
            PwValue basename2 = PW_NULL;
            if (!pw_basename(&s2, &basename2)) {
                panic();
            }
            //pw_dump(stderr, &basename2);
            TEST(pw_equal(&basename2, "blahblahblah"));
        }
    }

    // compare line readers
    {
        static char* sample_files[] = {
            "./test/data/sample.json",
            "./test/data/sample-no-trailing-lf.json"
        };
        for (unsigned i = 0; i < PW_LENGTH(sample_files); i++) {{

            PwValue file_name = PwStaticString(sample_files[i]);

            off_t file_size;
            if (!pw_file_size(&file_name, &file_size)) {
                panic();
            }

            // read content from file

            char data[file_size + 1];
            bzero(data, sizeof(data));

            PwValue file = PW_NULL;
            if (!pw_file_open(&file_name, O_RDONLY, 0, &file)) {
                panic();
            }

            unsigned bytes_read;
            if (!pw_read(&file, data, sizeof(data), &bytes_read)) {
                panic();
            }
            TEST(bytes_read == file_size);
            data[file_size] = 0;

            // reopen file
            if (!pw_file_open(&file_name, O_RDONLY, 0, &file)) {
                panic();
            }

            // create string IO to compare with

            PwValue str_io = PW_NULL;
            if (!pw_create_string_io(data, &str_io)) {
                panic();
            }

            if (!pw_start_read_lines(&file)) {
                panic();
            }
            if (!pw_start_read_lines(&str_io)) {
                panic();
            }

            PwValue line_f = PW_STRING("");
            PwValue line_s = PW_STRING("");

            for (;;) {{
                PwValue status_f = PW_NULL;
                if (!pw_read_line_inplace(&file, &line_f)) {
                    pw_clone2(&current_task->status, &status_f);
                }
                PwValue status_s = PW_NULL;
                if (!pw_read_line_inplace(&str_io, &line_s)) {
                    pw_clone2(&current_task->status, &status_s);
                }
                TEST(pw_equal(&status_f, &status_s));
                if (!pw_equal(&status_f, &status_s)) {
                    fprintf(stderr, "%s -- Line number: %u (file), %u (string I/O)\n",
                            sample_files[i],
                            pw_get_line_number(&file),
                            pw_get_line_number(&str_io)
                    );
                    pw_dump(stderr, &status_f);
                    pw_dump(stderr, &line_f);
                    pw_dump(stderr, &status_s);
                    pw_dump(stderr, &line_s);
                    break;
                }
                if (pw_is_eof()) {
                    break;
                }
                TEST(pw_is_null(&status_f));
                if (!pw_is_null(&status_f)) {
                    break;
                }
                TEST(pw_equal(&line_f, &line_s));
                if (!pw_equal(&line_f, &line_s)) {
                    fprintf(stderr, "%s -- Line number: %u (file), %u (string I/O)\n",
                            sample_files[i],
                            pw_get_line_number(&file),
                            pw_get_line_number(&str_io)
                    );
                    pw_dump(stderr, &line_f);
                    pw_dump(stderr, &line_s);
                    break;
                }
                //fprintf(stderr, "Line %u\n", pw_get_line_number(&file));
                //pw_dump(stderr, &line_f);
            }}
        }}
    }
}

void test_string_io()
{
    PwValue sio = PW_NULL;
    if (!pw_create_string_io("one\ntwo\nthree", &sio)) {
        panic();
    }
    {
        PwValue line = PW_NULL;
        if (!pw_read_line(&sio, &line)) {
            panic();
        }
        TEST(pw_equal(&line, "one\n"));
    }
    {
        PwValue line = PW_STRING("");
        if (!pw_read_line_inplace(&sio, &line)) {
            panic();
        }
        TEST(pw_equal(&line, "two\n"));

        // push back
        {
            bool status = pw_unread_line(&sio, &line);
            TEST(status);
        }
    }
    {
        // read pushed back
        PwValue line = PW_STRING("");
        if (!pw_read_line_inplace(&sio, &line)) {
            panic();
        }
        TEST(pw_equal(&line, "two\n"));
    }
    {
        PwValue line = PW_STRING("");
        if (!pw_read_line_inplace(&sio, &line)) {
            panic();
        }
        TEST(pw_equal(&line, "three"));
    }
    {
        // EOF
        PwValue line = PW_STRING("");
        TEST(!pw_read_line_inplace(&sio, &line));
        TEST(pw_is_eof());
    }
    // start over again
    {
        if (!pw_start_read_lines(&sio)) {
            panic();
        }
        PwValue line = PW_NULL;
        if (!pw_read_line(&sio, &line)) {
            panic();
        }
        TEST(pw_equal(&line, "one\n"));
    }
}

void test_netutils()
{
    {
        PwValue addr = PW_NULL;
        if (!pw_create_string("192.168.0.1:8080", &addr)) {
            panic();
        }
        PwValue parsed_addr = PW_NULL;
        if (!pw_parse_inet_address(&addr, &parsed_addr)) {;
            panic();
        }
        TEST(pw_sockaddr_ipv4(&parsed_addr) == (192U << 24) + (168U << 16) + 1U);
        TEST(pw_sockaddr_port(&parsed_addr) == 8080);
    }
/*
    {
        PwValue addr = PW_NULL;
        if (!pw_create_string("2001:db8:85a3:8d3:1319:8a2e:370:7348", &addr)) {
            panic();
        }
        PwValue parsed_addr = PW_NULL;
        if (!pw_parse_inet_address(&addr, &parsed_addr)) {
            panic();
        }
        pw_dump(stderr, &parsed_addr);
        TEST(memcmp(pw_sockaddr_ipv6(&parsed_addr), "\x20\x01\x0d\xb8\x85\xa3\x08\xd3\x13\x19\x8a\x2e\x03\x70\x73\x48", 16));
        TEST(pw_sockaddr_port(&parsed_addr) == 0);
    }
    {
        PwValue addr = PW_NULL;
        if (!pw_create_string("[2001:db8:85a3:8d3:1319:8a2e:370:7348]:443", &addr)) {
            panic();
        }
        PwValue parsed_addr = PW_NULL;
        if (!pw_parse_inet_address(&addr, &parsed_addr)) {
            panic();
        }
        pw_dump(stderr, &parsed_addr);
        TEST(memcmp(pw_sockaddr_ipv6(&parsed_addr), "\x20\x01\x0d\xb8\x85\xa3\x08\xd3\x13\x19\x8a\x2e\x03\x70\x73\x48", 16));
        TEST(pw_sockaddr_port(&parsed_addr) == 443);
    }
*/
    {
        PwValue subnet = PW_NULL;
        if (!pw_create_string("192.168.0.0/24", &subnet)) {
            panic();
        }
        PwValue netmask = PW_NULL;
        PwValue parsed_subnet = PW_NULL;
        if (!pw_parse_subnet(&subnet, &netmask, &parsed_subnet)) {
            panic();
        }
        TEST(pw_sockaddr_ipv4(&parsed_subnet) == (192U << 24) + (168U << 16));
        TEST(pw_sockaddr_netmask(&parsed_subnet) == 24);
    }
    {
        PwValue subnet = PW_NULL;
        if (!pw_create_string("192.168.0.0", &subnet)) {
            panic();
        }
        PwValue netmask = PW_NULL;
        if (!pw_create_string("255.255.255.0", &netmask)) {
            panic();
        }
        PwValue parsed_subnet = PW_NULL;
        if (!pw_parse_subnet(&subnet, &netmask, &parsed_subnet)) {
            panic();
        }
        TEST(pw_sockaddr_ipv4(&parsed_subnet) == (192U << 24) + (168U << 16));
        TEST(pw_sockaddr_netmask(&parsed_subnet) == 24);
        //pw_dump(stderr, &parsed_subnet);
    }
    {
        // prefer CIDR netmask
        PwValue subnet = PW_NULL;
        if (!pw_create_string("192.168.0.0/8", &subnet)) {
            panic();
        }
        PwValue netmask = PW_NULL;
        if (!pw_create_string("255.255.255.0", &netmask)) {
            panic();
        }
        PwValue parsed_subnet = PW_NULL;
        if (!pw_parse_subnet(&subnet, &netmask, &parsed_subnet)) {
            panic();
        }
        TEST(pw_sockaddr_ipv4(&parsed_subnet) == (192U << 24) + (168U << 16));
        TEST(pw_sockaddr_netmask(&parsed_subnet) == 8);
    }
    {
        // bad IP address
        PwValue subnet = PW_NULL;
        if (!pw_create_string("392.168.0.0/24", &subnet)) {
            panic();
        }
        PwValue netmask = PW_NULL;
        PwValue parsed_subnet = PW_NULL;
        TEST(!pw_parse_subnet(&subnet, &netmask, &parsed_subnet));
        TEST(current_task->status.status_code == PW_ERROR_BAD_IP_ADDRESS);
        //pw_dump(stderr, &parsed_subnet);
    }
    {
        // bad CIDR netmask
        PwValue subnet = PW_NULL;
        if (!pw_create_string("192.168.0.0/124", &subnet)) {
            panic();
        }
        PwValue netmask = PW_NULL;
        PwValue parsed_subnet = PW_NULL;
        TEST(!pw_parse_subnet(&subnet, &netmask, &parsed_subnet));
        TEST(current_task->status.status_code == PW_ERROR_BAD_NETMASK);
        //pw_dump(stderr, &parsed_subnet);
    }
    {
        // bad CIDR netmask
        PwValue subnet = PW_NULL;
        if (!pw_create_string("192.168.0.0/24/12", &subnet)) {
            panic();
        }
        PwValue netmask = PW_NULL;
        PwValue parsed_subnet = PW_NULL;
        TEST(!pw_parse_subnet(&subnet, &netmask, &parsed_subnet));
        TEST(current_task->status.status_code == PW_ERROR_BAD_NETMASK);
        //pw_dump(stderr, &parsed_subnet);
    }

    // pw_split_addr_port
    {
        PwValue addr_port = PW_NULL;
        if (!pw_create_string("example.com:80", &addr_port)) {
            panic();
        }
        PwValue addr = PW_NULL;
        PwValue port = PW_NULL;
        if (!pw_split_addr_port(&addr_port, &addr, &port)) {
            panic();
        }
        TEST(pw_equal(&addr, "example.com"));
        TEST(pw_equal(&port, "80"));
    }
    {
        PwValue addr_port = PW_NULL;
        if (!pw_create_string("80", &addr_port)) {
            panic();
        }
        PwValue addr = PW_NULL;
        PwValue port = PW_NULL;
        if (!pw_split_addr_port(&addr_port, &addr, &port)) {
            panic();
        }
        TEST(pw_equal(&addr, ""));
        TEST(pw_equal(&port, "80"));
    }
    {
        PwValue addr_port = PW_NULL;
        if (!pw_create_string("::1", &addr_port)) {
            panic();
        }
        PwValue addr = PW_NULL;
        PwValue port = PW_NULL;
        if (!pw_split_addr_port(&addr_port, &addr, &port)) {
            panic();
        }
        TEST(pw_equal(&addr, "::1"));
        TEST(pw_equal(&port, ""));
    }
    {
        PwValue addr_port = PW_NULL;
        if (!pw_create_string("[2001:db8:85a3:8d3:1319:8a2e:370:7348]:443", &addr_port)) {
            panic();
        }
        PwValue addr = PW_NULL;
        PwValue port = PW_NULL;
        if (!pw_split_addr_port(&addr_port, &addr, &port)) {
            panic();
        }
        TEST(pw_equal(&addr, "[2001:db8:85a3:8d3:1319:8a2e:370:7348]"));
        TEST(pw_equal(&port, "443"));
    }
}

void test_args()
{
    {
        char* argv[] = {
            "/bin/sh",
            "foo=bar",
            "one=1",
            "two",
            "three",
            "four=4"
        };
        PwValue args = PW_NULL;
        if (!pw_parse_kvargs(PW_LENGTH(argv), argv, &args)) {
            panic();
        }
        TEST(pw_is_map(&args));
        PwValue k = PW_NULL;
        PwValue v = PW_NULL;
        for(unsigned i = 0; i < PW_LENGTH(argv); i++) {
            TEST(pw_map_item(&args, i, &k, &v));
            if (i == 0) {
                TEST(pw_equal(&k, 0));
                TEST(pw_equal(&v, argv[i]));
            } else {
                char* separator = strchr(argv[i], '=');
                if (separator) {
                    unsigned n = separator - argv[i];
                    char key[n + 1];
                    strncpy(key, argv[i], n);
                    key[n] = 0;
                    TEST(pw_equal(&k, key));
                    TEST(pw_equal(&v, separator + 1));
                } else {
                    TEST(pw_equal(&k, argv[i]));
                    TEST(pw_is_null(&v));
                }
            }
        }
        //pw_dump(stderr, &args);
    }
}

void test_json()
{
    PwValue value = PW_NULL;
    if (!pw_array_va(&value,
        PwString("this"),
        PwString("is"),
        PwString("a"),
        pwva_map(
            PwString("number"),
            PwSigned(1),
            PwString("list"),
            pwva_array(
                PwString("one"),
                PwString("two"),
                pwva_map(
                    PwString("three"),
                        pwva_array(
                            PwSigned(1),
                            PwSigned(2),
                            pwva_map( PwString("four"), PwString("five\nsix\n") )
                        )
                )
            )
        ),
        PwString("daz good")
    )) {
        panic();
    }
    {
        PwValue result = PW_NULL;
        if (!pw_to_json(&value, 0, &result)) {
            panic();
        }
        PwValue reference = PW_NULL;
        if (!pw_create_string(
            "[\"this\",\"is\",\"a\",{\"number\":1,\"list\":[\"one\",\"two\",{\"three\":[1,2,{\"four\":\"five\\nsix\\n\"}]}]},\"daz good\"]",
            &reference
        )) {
            panic();
        }
        //pw_dump(stderr, &result);
        //pw_dump(stderr, &reference);
        //PW_CSTRING_LOCAL(json, &result);
        //fprintf(stderr, "%s\n", json);
        TEST(pw_equal(&result, &reference));
    }
    {
        PwValue result = PW_NULL;
        if (!pw_to_json(&value, 4, &result)) {
            panic();
        }
        PwValue reference = PW_NULL;
        if (!pw_create_string(
            "[\n"
            "    \"this\",\n"
            "    \"is\",\n"
            "    \"a\",\n"
            "    {\n"
            "        \"number\": 1,\n"
            "        \"list\": [\n"
            "            \"one\",\n"
            "            \"two\",\n"
            "            {\"three\": [\n"
            "                1,\n"
            "                2,\n"
            "                {\"four\": \"five\\nsix\\n\"}\n"
            "            ]}\n"
            "        ]\n"
            "    },\n"
            "    \"daz good\"\n"
            "]",
            &reference
        )) {
            panic();
        }
        //pw_dump(stderr, &result);
        //PW_CSTRING_LOCAL(json, &result);
        //fprintf(stderr, "%s\n", json);
        TEST(pw_equal(&result, &reference));
    }
    // test pw_get
    {
        PwValue v = PW_NULL;
        if (!pw_get(&v, &value, "3", "number")) {
            panic();
        }
        //pw_dump(stderr, &v);
        TEST(pw_equal(&v, 1));

        PwValue v2 = PW_NULL;
        if (!pw_get(&v2, &value, "3", "list", "-1", "three", "1")) {
            panic();
        }
        //pw_dump(stderr, &v2);
        TEST(pw_equal(&v2, 2));

        PwValue v3 = PW_NULL;
        TEST(!pw_get(&v3, &value, "0", "that"));
    }
    // test pw_set
    {
        PwValue v = PwUnsigned(777);
        if (!pw_set(&v, &value, "3", "number")) {
            panic();
        }
        if (!pw_get(&v, &value, "3", "number")) {
            panic();
        }
        TEST(pw_equal(&v, 777));
    }{
        PwValue v = PwUnsigned(777);
        if (!pw_set(&v, &value, "3", "list", "-1", "three", "1")) {
            panic();
        }
        if (!pw_get(&v, &value, "3", "list", "-1", "three", "1")) {
            panic();
        }
        TEST(pw_equal(&v, 777));

        TEST(!pw_set(&v, &value, "0", "that"));
    }
}

void test_socket()
{
    {
        PwValue sock = PW_NULL;
        if (!pw_socket(PwTypeId_Socket, AF_LOCAL, SOCK_DGRAM, 0, &sock)) {
            panic();
        }
        //pw_dump(stderr, &sock);
        TEST(pw_is_socket(&sock));
        PwValue local_addr = PW_STRING("\0uwtest");
        if (!pw_socket_bind(&sock, &local_addr)) {
            panic();
        }
        //pw_dump(stderr, &status);
    }
    {
        PwValue sock = PW_NULL;
        if (!pw_socket(PwTypeId_Socket, AF_INET, SOCK_STREAM, 0, &sock)) {
            panic();
        }
        //pw_dump(stderr, &sock);
        TEST(pw_is_socket(&sock));
        PwValue local_addr = PW_STATIC_STRING("0.0.0.0:23451");
        if (!pw_socket_bind(&sock, &local_addr)) {
            panic();
        }
        //pw_dump(stderr, &status);
    }
    {
        PwValue sock = PW_NULL;
        if (!pw_socket(PwTypeId_Socket, AF_INET, SOCK_STREAM, 0, &sock)) {
            panic();
        }
        //pw_dump(stderr, &sock);
        TEST(pw_is_socket(&sock));
        PwValue local_addr = PW_NULL;
        if (!pw_parse_inet_address("0.0.0.0:23451", &local_addr)) {
            panic();
        }
        if (!pw_socket_bind(&sock, &local_addr)) {
            panic();
        }
        //pw_dump(stderr, &status);
    }
}

int main(int argc, char* argv[])
{
    //debug_allocator.verbose = true;
    //init_allocator(&debug_allocator);

    //init_allocator(&stdlib_allocator);

    //pet_allocator.trace = true;
    //pet_allocator.verbose = true;
    init_allocator(&pet_allocator);

    PwValue start_time = PW_NULL;
    if (!pw_monotonic(&start_time)) {
        panic();
    }

    test_icu();
    test_integral_types();
    test_string();
    test_array();
    test_map();
    test_file();
    test_string_io();
    test_netutils();
    test_args();
    test_json();
    test_socket();

    PwValue end_time = PW_NULL;
    if (!pw_monotonic(&end_time)) {
        panic();
    }
    PwValue timediff = pw_timestamp_diff(&end_time, &start_time);
    PwValue timediff_str = PW_NULL;
    TEST(pw_to_string(&timediff, &timediff_str));
    PW_CSTRING_LOCAL(timediff_cstr, &timediff_str);
    fprintf(stderr, "time elapsed: %s\n", timediff_cstr);

    if (num_fail == 0) {
        fprintf(stderr, "%d test%s OK\n", num_tests, (num_tests == 1)? "" : "s");
    }

    fprintf(stderr, "leaked blocks: %zu\n", default_allocator.stats->blocks_allocated);
}
