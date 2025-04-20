#include <time.h>

#include "include/pw.h"

PwResult pw_monotonic()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);

    __PWDECL_Timestamp(ts);
    ts.ts_seconds = t.tv_sec;
    ts.ts_nanoseconds = t.tv_nsec;
    return ts;
}

PwResult pw_timestamp_sum(PwValuePtr a, PwValuePtr b)
{
    pw_assert_timestamp(a);
    pw_assert_timestamp(b);

    __PWDECL_Timestamp(sum);

    sum.ts_seconds = a->ts_seconds + b->ts_seconds;
    sum.ts_nanoseconds = a->ts_nanoseconds - b->ts_nanoseconds;
    if (sum.ts_nanoseconds >= 1000'000'000UL) {
        sum.ts_nanoseconds -= 1000'000'000UL;
        sum.ts_seconds++;
    }
    return sum;
}

PwResult pw_timestamp_diff(PwValuePtr a, PwValuePtr b)
{
    pw_assert_timestamp(a);
    pw_assert_timestamp(b);

    __PWDECL_Timestamp(diff);

    diff.ts_seconds = a->ts_seconds - b->ts_seconds;
    diff.ts_nanoseconds = a->ts_nanoseconds - b->ts_nanoseconds;
    if (a->ts_nanoseconds < b->ts_nanoseconds) {
        diff.ts_seconds--;
        diff.ts_nanoseconds += 1000'000'000UL;
    }
    return diff;
}
