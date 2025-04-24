#include <stdio.h>
#include <string.h>

#include "include/pw.h"
#include "include/pw_args.h"
#include "include/pw_datetime.h"
#include "include/pw_netutils.h"
#include "include/pw_socket.h"
#include "include/pw_to_json.h"
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
    PwValue null_1 = PwNull();
    PWDECL_Null(null_2);
    TEST(pw_is_null(&null_1));
    TEST(pw_is_null(&null_2));

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
    TEST(pw_equal(&null_1, &null_2));
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
    TEST(pw_isspace(0) == false);

    { // testing char_size=1
        PwValue v = pw_create_empty_string(0, 1);
        TEST(_pw_string_length(&v) == 0);
        TEST(_pw_string_capacity(&v) == 12);
        TEST(_pw_string_char_size(&v) == 1);
        //pw_dump(stderr, &v);

        pw_string_append(&v, "hello");

        TEST(_pw_string_length(&v) == 5);
        TEST(_pw_string_capacity(&v) == 12);
        //pw_dump(stderr, &v);

        pw_string_append(&v, '!');

        TEST(_pw_string_length(&v) == 6);
        TEST(_pw_string_capacity(&v) == 12);
        //pw_dump(stderr, &v);

        // XXX TODO increase capacity to more than 64K
        for (int i = 0; i < 250; i++) {
            pw_string_append(&v, ' ');
        }
        TEST(_pw_string_length(&v) == 256);
        TEST(_pw_string_char_size(&v) == 1);
        //pw_dump(stderr, &v);

        pw_string_append(&v, "pet");
        pw_string_erase(&v, 5, 255);
        TEST(pw_equal(&v, "hello pet"));
        //pw_dump(stderr, &v);
        TEST(!pw_equal(&v, ""));

        // test comparison
        PwValue v2 = pw_create_string("hello pet");
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

        PwValue v3 = pw_create_string("hello pet");
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
        pw_string_erase(&v, 4, 255);
        TEST(pw_equal(&v, "hell"));

        pw_string_erase(&v, 0, 2);
        TEST(pw_equal(&v, "ll"));

        pw_string_truncate(&v, 0);
        TEST(pw_equal(&v, ""));

        TEST(_pw_string_length(&v) == 0);
        TEST(_pw_string_capacity(&v) == 264);
        //pw_dump(stderr, &v);

        // test append substring
        pw_string_append_substring(&v, "0123456789", 3, 7);
        TEST(pw_equal(&v, "3456"));
        pw_string_append_substring(&v, u8"0123456789", 3, 7);
        TEST(pw_equal(&v, "34563456"));
        pw_string_append_substring(&v, U"0123456789", 3, 7);
        TEST(pw_equal(&v, "345634563456"));
        pw_string_truncate(&v, 0);

        // change char size to 2-byte by appending wider chars -- the string will be copied
        pw_string_append(&v, u8"สวัสดี");

        TEST(_pw_string_length(&v) == 6);
        TEST(_pw_string_capacity(&v) == 268);  // capacity is slightly changed because of alignment and char_size increase
        TEST(_pw_string_char_size(&v) == 2);
        TEST(pw_equal(&v, u8"สวัสดี"));
        //pw_dump(stderr, &v);
    }

    { // testing char_size=2
        PwValue v = pw_create_empty_string(1, 2);
        TEST(_pw_string_length(&v) == 0);
        TEST(_pw_string_capacity(&v) == 6);
        TEST(_pw_string_char_size(&v) == 2);
        //pw_dump(stderr, &v);

        pw_string_append(&v, u8"สบาย");

        TEST(_pw_string_length(&v) == 4);
        TEST(_pw_string_capacity(&v) == 6);
        //pw_dump(stderr, &v);

        pw_string_append(&v, 0x0e14);
        pw_string_append(&v, 0x0e35);

        TEST(_pw_string_length(&v) == 6);
        TEST(_pw_string_capacity(&v) == 6);
        TEST(pw_equal(&v, u8"สบายดี"));
        //pw_dump(stderr, &v);

        // test truncate
        pw_string_truncate(&v, 4);
        TEST(pw_equal(&v, u8"สบาย"));
        TEST(!pw_equal(&v, ""));
        //pw_dump(stderr, &v);

        // increase capacity to 2 bytes
        for (int i = 0; i < 251; i++) {
            pw_string_append(&v, ' ');
        }
        TEST(_pw_string_length(&v) == 255);
        TEST(_pw_string_capacity(&v) == 260);
        TEST(_pw_string_char_size(&v) == 2);
        //pw_dump(stderr, &v);

        pw_string_append(&v, U"สบาย");
        pw_string_erase(&v, 4, 255);
        TEST(pw_equal(&v, u8"สบายสบาย"));
        TEST(!pw_equal(&v, ""));

        // test comparison
        PwValue v2 = pw_create_string(u8"สบายสบาย");
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
        pw_string_erase(&v, 4, 255);
        TEST(pw_equal(&v, u8"สบาย"));

        pw_string_erase(&v2, 0, 4);
        TEST(pw_equal(&v, u8"สบาย"));

        pw_string_truncate(&v, 0);
        TEST(pw_equal(&v, ""));

        // test append substring
        pw_string_append_substring(&v, u8"สบายสบาย", 1, 4);
        TEST(pw_equal(&v, u8"บาย"));
        pw_string_append_substring(&v, U"สบายสบาย", 1, 4);
        TEST(pw_equal(&v, U"บายบาย"));
        pw_string_truncate(&v, 0);

        TEST(_pw_string_length(&v) == 0);
        TEST(_pw_string_capacity(&v) == 260);
        //pw_dump(stderr, &v);
    }

    { // testing char_size=3
        PwValue v = pw_create_empty_string(1, 3);
        TEST(_pw_string_length(&v) == 0);
        TEST(_pw_string_capacity(&v) == 4);
        TEST(_pw_string_char_size(&v) == 3);
        //pw_dump(stderr, &v);
    }

    { // testing char_size=4
        PwValue v = pw_create_empty_string(1, 4);
        TEST(_pw_string_length(&v) == 0);
        TEST(_pw_string_capacity(&v) == 3);
        TEST(_pw_string_char_size(&v) == 4);
        //pw_dump(stderr, &v);
    }

    { // test trimming
        PwValue v = pw_create_string(u8"  สวัสดี   ");
        TEST(pw_strlen(&v) == 11);
        pw_string_ltrim(&v);
        TEST(pw_equal(&v, u8"สวัสดี   "));
        pw_string_rtrim(&v);
        TEST(pw_equal(&v, u8"สวัสดี"));
        TEST(pw_strlen(&v) == 6);
    }

    { // test pw_strcat (by value)
        PwValue v = pw_strcat(
            pw_create_string("Hello! "), PwCharPtr("Thanks"), PwChar32Ptr(U"🙏"), PwCharPtr(u8"สวัสดี")
        );
        TEST(pw_equal(&v, U"Hello! Thanks🙏สวัสดี"));
        //pw_dump(stderr, &v);
    }

    { // test pw_strcat (by reference)
        PwValue s1 = pw_create_string("Hello! ");
        PwValue s2 = PwCharPtr("Thanks");
        PwValue s3 = PwChar32Ptr(U"🙏");
        PwValue s4 = PwCharPtr(u8"สวัสดี");
        PwValue v = pw_strcat(&s1, &s2, &s3, &s4);
        TEST(pw_equal(&v, U"Hello! Thanks🙏สวัสดี"));
        //pw_dump(stderr, &v);
    }

    { // test split/join
        PwValue str = pw_create_string(U"สบาย/สบาย/yo/yo");
        PwValue array = pw_string_split_chr(&str, '/', 0);
        //pw_dump(stderr, &array);
        PwValue array2 = pw_string_rsplit_chr(&str, '/', 1);
        //pw_dump(stderr, &array2);
        PwValue first = pw_array_item(&array2, 0);
        PwValue last = pw_array_item(&array2, 1);
        TEST(pw_equal(&first, U"สบาย/สบาย/yo"));
        TEST(pw_equal(&last, "yo"));
        PwValue array3 = pw_string_split_chr(&str, '/', 1);
        //pw_dump(stderr, &array3);
        PwValue first3 = pw_array_item(&array3, 0);
        PwValue last3 = pw_array_item(&array3, 1);
        TEST(pw_equal(&first3, U"สบาย"));
        TEST(pw_equal(&last3, U"สบาย/yo/yo"));
        PwValue v = pw_array_join('/', &array);
        TEST(pw_equal(&v, U"สบาย/สบาย/yo/yo"));
    }

    // test append_buffer
    {
        char8_t data[2500];
        memset(data, '1', sizeof(data));

        PWDECL_String(str);

        pw_string_append_buffer(&str, data, sizeof(data));
        TEST(_pw_string_capacity(&str) >= _pw_string_length(&str));
        TEST(_pw_string_length(&str) == 2500);
        //pw_dump(stderr, &str);
    }

    { // test startswith/endswith
        PwValue str = pw_create_string("hello world");
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
        PwValue str = pw_create_string(u8"ความคืบหน้า");
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

    { // test isdigit
        PwValue empty = PwString();
        PwValue nondigit = pw_create_string(u8"123รูปโป๊");
        PwValue digit = pw_create_string("456");
        TEST(!pw_string_isdigit(&empty));
        TEST(!pw_string_isdigit(&nondigit));
        TEST(pw_string_isdigit(&digit));
    }
}

void test_array()
{
    PwValue array = PwArray();

    TEST(pw_array_length(&array) == 0);

    for(unsigned i = 0; i < 1000; i++) {
        {
            PwValue item = PwUnsigned(i);
            pw_array_append(&array, &item);
        }

        TEST(pw_array_length(&array) == i + 1);

        {
            PwValue v = pw_array_item(&array, i);
            PwValue item = PwUnsigned(i);
            TEST(pw_equal(&v, &item));
        }
    }

    {
        PwValue item = pw_array_item(&array, -2);
        PwValue v = PwUnsigned(998);
        TEST(pw_equal(&v, &item));
    }

    pw_array_del(&array, 100, 200);
    TEST(pw_array_length(&array) == 900);

    {
        PwValue item = pw_array_item(&array, 99);
        PwValue v = PwUnsigned(99);
        TEST(pw_equal(&v, &item));
    }
    {
        PwValue item = pw_array_item(&array, 100);
        PwValue v = PwUnsigned(200);
        TEST(pw_equal(&v, &item));
    }

    {
        PwValue slice = pw_array_slice(&array, 750, 850);
        TEST(pw_array_length(&slice) == 100);
        {
            PwValue item = pw_array_item(&slice, 1);
            TEST(pw_equal(&item, 851));
        }
        {
            PwValue item = pw_array_item(&slice, 98);
            TEST(pw_equal(&item, 948));
        }
    }

    PwValue pulled = pw_array_pull(&array);
    TEST(pw_equal(&pulled, 0));
    TEST(pw_array_length(&array) == 899);
    pulled = pw_array_pull(&array);
    TEST(pw_equal(&pulled, 1));
    TEST(pw_array_length(&array) == 898);

    { // test join
        PwValue array = PwArray();
        pw_array_append(&array, "Hello");
        pw_array_append(&array, u8"สวัสดี");
        pw_array_append(&array, "Thanks");
        pw_array_append(&array, U"mulțumesc");
        PwValue v = pw_array_join('/', &array);
        TEST(pw_equal(&v, U"Hello/สวัสดี/Thanks/mulțumesc"));
        //pw_dump(stderr, &v);
    }

    { // test join with CharPtr
        PwValue array       = PwArray();
        PwValue sawatdee   = PwCharPtr(u8"สวัสดี");
        PwValue thanks     = PwCharPtr("Thanks");
        PwValue multsumesc = PwChar32Ptr(U"mulțumesc");
        PwValue wat        = PwChar32Ptr(U"🙏");
        pw_array_append(&array, "Hello");
        pw_array_append(&array, &sawatdee);
        pw_array_append(&array, &thanks);
        pw_array_append(&array, &multsumesc);
        PwValue v = pw_array_join(&wat, &array);
        TEST(pw_equal(&v, U"Hello🙏สวัสดี🙏Thanks🙏mulțumesc"));
        //pw_dump(stderr, &v);
    }

    { // test dedent
        PwValue array = PwArray(
            PwCharPtr("   first line"),
            PwCharPtr("  second line"),
            PwCharPtr("    third line")
        );
        pw_array_dedent(&array);
        PwValue v = pw_array_join(',', &array);
        TEST(pw_equal(&v, " first line,second line,  third line"));
        //pw_dump(stderr, &v);
    }
}

void test_map()
{
    {
        PwValue map = PwMap();
        PwValue key = PwUnsigned(0);
        PwValue value = PwBool(false);

        pw_map_update(&map, &key, &value);
        TEST(pw_map_length(&map) == 1);
        pw_destroy(&key);
        pw_destroy(&value);

        key = PwUnsigned(0);
        TEST(pw_map_has_key(&map, &key));
        pw_destroy(&key);

        key = PwNull();
        TEST(!pw_map_has_key(&map, &key));
        pw_destroy(&key);

        for (int i = 1; i < 50; i++) {
            key = PwUnsigned(i);
            value = PwUnsigned(i);
            pw_map_update(&map, &key, &value);
            pw_destroy(&key);
            pw_destroy(&value);
        }
        pw_map_del(&map, 25);

        TEST(pw_map_length(&map) == 49);
        //pw_dump(stderr, &map);
    }

    {
        // XXX CType leftovers
        PwValue map = PwMap(
            PwCharPtr("let's"),       PwCharPtr("go!"),
            PwNull(),                 PwBool(true),
            PwBool(true),             PwCharPtr("true"),
            PwSigned(-10),            PwBool(false),
            PwSigned('b'),            PwSigned(-42),
            PwUnsigned(100),          PwSigned(-1000000L),
            PwUnsigned(300000000ULL), PwFloat(1.23),
            PwCharPtr(u8"สวัสดี"),      PwChar32Ptr(U"สบาย"),
            PwCharPtr("finally"),     PwMap(PwCharPtr("ok"), PwCharPtr("done"))
        );
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

        PwValue file = pw_file_open(data_filename, O_RDONLY, 0);
        TEST(pw_ok(&file));
        PwValue status = pw_start_read_lines(&file);
        TEST(pw_ok(&status));
        PwValue line = pw_create_string("");
        for (;;) {{
            PwValue status = pw_read_line_inplace(&file, &line);
            TEST(pw_ok(&status));
            if (pw_error(&status)) {
                return;
            }
            if (!pw_equal(&line, a)) {
                break;
            }
        }}
        TEST(pw_equal(&line, b));
        {
            PwValue status = pw_read_line_inplace(&file, &line);
            TEST(pw_ok(&status));
        }
        TEST(pw_equal(&line, c));

        { // test path functions
            PwValue s = pw_create_string("/bin/bash");
            PwValue basename = pw_basename(&s);
            //pw_dump(stderr, &basename);
            TEST(pw_equal(&basename, "bash"));
            PwValue dirname = pw_dirname(&s);
            //pw_dump(stderr, &dirname);
            TEST(pw_equal(&dirname, "/bin"));
            PwValue path = pw_path(PwCharPtr(""), PwCharPtr("bin"), PwCharPtr("bash"));
            TEST(pw_equal(&path, "/bin/bash"));
            //pw_dump(stderr, &path);
            PwValue part1 = pw_create_string("");
            PwValue part2 = pw_create_string("bin");
            PwValue part3 = pw_create_string("bash");
            PwValue path2 = pw_path(&part1, &part2, &part3);
            TEST(pw_equal(&path2, "/bin/bash"));

            PwValue s2 = pw_create_string("blahblahblah");
            //pw_dump(stderr, &s2);
            PwValue basename2 = pw_basename(&s2);
            //pw_dump(stderr, &basename2);
            TEST(pw_equal(&basename2, "blahblahblah"));
        }
    }

    // compare line readers
    {
        char* sample_files[] = {
            "./test/data/sample.json",
            "./test/data/sample-no-trailing-lf.json"
        };
        for (unsigned i = 0; i < PW_LENGTH(sample_files); i++) {{

            PwValue file_name = PwCharPtr(sample_files[i]);

            PwValue file_size = pw_file_size(&file_name);
            TEST(pw_is_unsigned(&file_size));
            if (!pw_is_unsigned(&file_size)) {
                pw_dump(stderr, &file_size);
            }

            // read content from file

            char data[file_size.unsigned_value + 1];
            bzero(data, sizeof(data));

            PwValue file = pw_file_open(&file_name, O_RDONLY, 0);
            TEST(pw_ok(&file));

            unsigned bytes_read;
            PwValue status = pw_read(&file, data, sizeof(data), &bytes_read);
            TEST(pw_ok(&status));
            TEST(bytes_read == file_size.unsigned_value);
            data[file_size.unsigned_value] = 0;

            // reopen file
            pw_destroy(&file);
            file = pw_file_open(&file_name, O_RDONLY, 0);
            TEST(pw_ok(&file));

            // create string IO to compare with

            PwValue str_io = pw_create_string_io(data);

            status = pw_start_read_lines(&file);
            TEST(pw_ok(&status));
            status = pw_start_read_lines(&str_io);
            TEST(pw_ok(&status));

            PwValue line_f = pw_create_string("");
            PwValue line_s = pw_create_string("");
            for (;;) {{
                PwValue status_f = pw_read_line_inplace(&file, &line_f);
                PwValue status_s = pw_read_line_inplace(&str_io, &line_s);
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
                if (pw_eof(&status_f)) {
                    break;
                }
                TEST(pw_ok(&status_f));
                if (pw_error(&status_f)) {
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
    PwValue sio = pw_create_string_io("one\ntwo\nthree");
    {
        PwValue line = pw_read_line(&sio);
        TEST(pw_equal(&line, "one\n"));
    }
    {
        PwValue line = PwString();
        PwValue status = pw_read_line_inplace(&sio, &line);
        TEST(pw_ok(&status));
        TEST(pw_equal(&line, "two\n"));

        // push back
        {
            bool status = pw_unread_line(&sio, &line);
            TEST(status);
        }
    }
    {
        // read pushed back
        PwValue line = pw_create_string("");
        PwValue status = pw_read_line_inplace(&sio, &line);
        TEST(pw_ok(&status));
        TEST(pw_equal(&line, "two\n"));
    }
    {
        PWDECL_String(line);
        PwValue status = pw_read_line_inplace(&sio, &line);
        TEST(pw_ok(&status));
        TEST(pw_equal(&line, "three"));
    }
    {
        // EOF
        PWDECL_String(line);
        PwValue status = pw_read_line_inplace(&sio, &line);
        TEST(pw_error(&status));
    }
    // start over again
    {
        PwValue status = pw_start_read_lines(&sio);
        TEST(pw_ok(&status));
        PwValue line = pw_read_line(&sio);
        TEST(pw_equal(&line, "one\n"));
    }
}

void test_netutils()
{
    {
        PwValue addr = pw_create_string("192.168.0.1:8080");
        PwValue parsed_addr = pw_parse_inet_address(&addr);
        TEST(pw_sockaddr_ipv4(&parsed_addr) == (192U << 24) + (168U << 16) + 1U);
        TEST(pw_sockaddr_port(&parsed_addr) == 8080);
    }
/*
    {
        PwValue addr = pw_create_string("2001:db8:85a3:8d3:1319:8a2e:370:7348");
        PwValue parsed_addr = pw_parse_inet_address(&addr);
        pw_dump(stderr, &parsed_addr);
        TEST(memcmp(pw_sockaddr_ipv6(&parsed_addr), "\x20\x01\x0d\xb8\x85\xa3\x08\xd3\x13\x19\x8a\x2e\x03\x70\x73\x48", 16));
        TEST(pw_sockaddr_port(&parsed_addr) == 0);
    }
    {
        PwValue addr = pw_create_string("[2001:db8:85a3:8d3:1319:8a2e:370:7348]:443");
        PwValue parsed_addr = pw_parse_inet_address(&addr);
        pw_dump(stderr, &parsed_addr);
        TEST(memcmp(pw_sockaddr_ipv6(&parsed_addr), "\x20\x01\x0d\xb8\x85\xa3\x08\xd3\x13\x19\x8a\x2e\x03\x70\x73\x48", 16));
        TEST(pw_sockaddr_port(&parsed_addr) == 443);
    }
*/
    {
        PwValue subnet = pw_create_string("192.168.0.0/24");
        PwValue netmask = PwNull();
        PwValue parsed_subnet = pw_parse_subnet(&subnet, &netmask);
        TEST(pw_sockaddr_ipv4(&parsed_subnet) == (192U << 24) + (168U << 16));
        TEST(pw_sockaddr_netmask(&parsed_subnet) == 24);
    }
    {
        PwValue subnet = pw_create_string("192.168.0.0");
        PwValue netmask = pw_create_string("255.255.255.0");
        PwValue parsed_subnet = pw_parse_subnet(&subnet, &netmask);
        TEST(pw_sockaddr_ipv4(&parsed_subnet) == (192U << 24) + (168U << 16));
        TEST(pw_sockaddr_netmask(&parsed_subnet) == 24);
        //pw_dump(stderr, &parsed_subnet);
    }
    {
        // prefer CIDR netmask
        PwValue subnet = pw_create_string("192.168.0.0/8");
        PwValue netmask = pw_create_string("255.255.255.0");
        PwValue parsed_subnet = pw_parse_subnet(&subnet, &netmask);
        TEST(pw_sockaddr_ipv4(&parsed_subnet) == (192U << 24) + (168U << 16));
        TEST(pw_sockaddr_netmask(&parsed_subnet) == 8);
    }
    {
        // bad IP address
        PwValue subnet = pw_create_string("392.168.0.0/24");
        PwValue netmask = PwNull();
        PwValue parsed_subnet = pw_parse_subnet(&subnet, &netmask);
        TEST(pw_error(&parsed_subnet));
        TEST(parsed_subnet.status_code == PW_ERROR_BAD_IP_ADDRESS);
        //pw_dump(stderr, &parsed_subnet);
    }
    {
        // bad CIDR netmask
        PwValue subnet = pw_create_string("192.168.0.0/124");
        PwValue netmask = PwNull();
        PwValue parsed_subnet = pw_parse_subnet(&subnet, &netmask);
        TEST(parsed_subnet.status_code == PW_ERROR_BAD_NETMASK);
        //pw_dump(stderr, &parsed_subnet);
    }
    {
        // bad CIDR netmask
        PwValue subnet = pw_create_string("192.168.0.0/24/12");
        PwValue netmask = PwNull();
        PwValue parsed_subnet = pw_parse_subnet(&subnet, &netmask);
        TEST(parsed_subnet.status_code == PW_ERROR_BAD_NETMASK);
        //pw_dump(stderr, &parsed_subnet);
    }

    // pw_split_addr_port
    {
        PwValue addr_port = pw_create_string("example.com:80");
        PwValue parts = pw_split_addr_port(&addr_port);
        PwValue addr = pw_array_item(&parts, 0);
        PwValue port = pw_array_item(&parts, 1);
        TEST(pw_equal(&addr, "example.com"));
        TEST(pw_equal(&port, "80"));
    }
    {
        PwValue addr_port = pw_create_string("80");
        PwValue parts = pw_split_addr_port(&addr_port);
        PwValue addr = pw_array_item(&parts, 0);
        PwValue port = pw_array_item(&parts, 1);
        TEST(pw_equal(&addr, ""));
        TEST(pw_equal(&port, "80"));
    }
    {
        PwValue addr_port = pw_create_string("::1");
        PwValue parts = pw_split_addr_port(&addr_port);
        PwValue addr = pw_array_item(&parts, 0);
        PwValue port = pw_array_item(&parts, 1);
        TEST(pw_equal(&addr, "::1"));
        TEST(pw_equal(&port, ""));
    }
    {
        PwValue addr_port = pw_create_string("[2001:db8:85a3:8d3:1319:8a2e:370:7348]:443");
        PwValue parts = pw_split_addr_port(&addr_port);
        PwValue addr = pw_array_item(&parts, 0);
        PwValue port = pw_array_item(&parts, 1);
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
        PwValue args = pw_parse_kvargs(
            PW_LENGTH(argv),
            argv
        );
        TEST(pw_is_map(&args));
        for(unsigned i = 0; i < PW_LENGTH(argv); i++) {{
            PwValue k = PwNull();
            PwValue v = PwNull();
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
        }}
        //pw_dump(stderr, &args);
    }
}

void test_json()
{
    PwValue value = PwArray(
        PwString_1_12(4, 't', 'h', 'i', 's', 0,0,0,0,0,0,0,0),
        PwString_1_12(2, 'i', 's', 0,0,0,0,0,0,0,0,0,0),
        PwString_1_12(1, 'a', 0,0,0,0,0,0,0,0,0,0,0),
        PwMap(
            PwString_1_12(6, 'n', 'u', 'm', 'b', 'e', 'r',0,0,0,0,0,0),
            PwSigned(1),
            PwString_1_12(4, 'l', 'i', 's', 't', 0,0,0,0,0,0,0,0),
            PwArray(
                PwString_1_12(3, 'o', 'n', 'e', 0,0,0,0,0,0,0,0,0),
                PwString_1_12(3, 't', 'w', 'o', 0,0,0,0,0,0,0,0,0),
                PwMap(
                    PwString_1_12(5, 't', 'h', 'r', 'e', 'e', 0,0,0,0,0,0,0),
                        PwArray(
                            PwSigned(1),
                            PwSigned(2),
                            PwMap( pw_create_string("four"), pw_create_string("five\nsix\n"))
                        )
                )
            )
        ),
        PwString_1_12(8, 'd', 'a', 'z', ' ', 'g', 'o', 'o', 'd', 0,0,0,0)
    );
    {
        // test pw_get

        PwValue v = pw_get(&value, "3", "list", "-1", "three", "1");
        pw_dump(stderr, &v);
        TEST(pw_equal(&v, 2));

        PwValue v2 = pw_get(&value, "0", "that");
        TEST(pw_error(&v2));
    }
    {
        PwValue result = pw_to_json(&value, 0);
        PwValue reference = pw_create_string(
            "[\"this\",\"is\",\"a\",{\"number\":1,\"list\":[\"one\",\"two\",{\"three\":[1,2,{\"four\":\"five\\nsix\\n\"}]}]},\"daz good\"]"
        );
        //pw_dump(stderr, &result);
        //pw_dump(stderr, &reference);
        //PW_CSTRING_LOCAL(json, &result);
        //fprintf(stderr, "%s\n", json);
        TEST(pw_equal(&result, &reference));
    }
    {
        PwValue result = pw_to_json(&value, 4);
        PwValue reference = pw_create_string(
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
            "]"
        );
        //pw_dump(stderr, &result);
        //PW_CSTRING_LOCAL(json, &result);
        //fprintf(stderr, "%s\n", json);
        TEST(pw_equal(&result, &reference));
    }
}

void test_socket()
{
    {
        PwValue sock = pw_socket(PwTypeId_Socket, AF_LOCAL, SOCK_DGRAM, 0);
        //pw_dump(stderr, &sock);
        TEST(pw_is_socket(&sock));
        PWDECL_String_1_12(local_addr, 7, 0, 'u', 'w', 't', 'e', 's', 't', 0, 0, 0, 0, 0);
        PwValue status = pw_socket_bind(&sock, &local_addr);
        //pw_dump(stderr, &status);
        TEST(pw_ok(&status));
    }
    {
        PwValue sock = pw_socket(PwTypeId_Socket, AF_INET, SOCK_STREAM, 0);
        //pw_dump(stderr, &sock);
        TEST(pw_is_socket(&sock));
        PwValue local_addr = PwCharPtr("0.0.0.0:23451");
        PwValue status = pw_socket_bind(&sock, &local_addr);
        //pw_dump(stderr, &status);
        TEST(pw_ok(&status));
    }
    {
        PwValue sock = pw_socket(PwTypeId_Socket, AF_INET, SOCK_STREAM, 0);
        //pw_dump(stderr, &sock);
        TEST(pw_is_socket(&sock));
        PwValue local_addr = pw_parse_inet_address("0.0.0.0:23451");
        PwValue status = pw_socket_bind(&sock, &local_addr);
        //pw_dump(stderr, &status);
        TEST(pw_ok(&status));
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

    PwValue start_time = pw_monotonic();

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

    PwValue end_time = pw_monotonic();
    PwValue timediff = pw_timestamp_diff(&end_time, &start_time);
    PwValue timediff_str = pw_to_string(&timediff);
    PW_CSTRING_LOCAL(timediff_cstr, &timediff_str);
    fprintf(stderr, "time elapsed: %s\n", timediff_cstr);

    if (num_fail == 0) {
        fprintf(stderr, "%d test%s OK\n", num_tests, (num_tests == 1)? "" : "s");
    }

    fprintf(stderr, "leaked blocks: %zu\n", default_allocator.stats->blocks_allocated);
}
