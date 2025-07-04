#include <errno.h>
#include <time.h>

#include "include/pw.h"

[[nodiscard]] bool pw_monotonic(PwValuePtr result)
{
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t) == -1) {
        pw_set_status(PwErrno(errno));
        return false;
    }
    result->type_id = PwTypeId_Timestamp;
    result->ts_seconds = t.tv_sec;
    result->ts_nanoseconds = t.tv_nsec;
    return true;
}

[[nodiscard]] _PwValue pw_timestamp_sum(PwValuePtr a, PwValuePtr b)
{
    pw_assert_timestamp(a);
    pw_assert_timestamp(b);

    _PwValue sum = PW_TIMESTAMP;

    sum.ts_seconds = a->ts_seconds + b->ts_seconds;
    sum.ts_nanoseconds = a->ts_nanoseconds - b->ts_nanoseconds;
    if (sum.ts_nanoseconds >= 1000'000'000UL) {
        sum.ts_nanoseconds -= 1000'000'000UL;
        sum.ts_seconds++;
    }
    return sum;
}

[[nodiscard]] _PwValue pw_timestamp_diff(PwValuePtr a, PwValuePtr b)
{
    pw_assert_timestamp(a);
    pw_assert_timestamp(b);

    _PwValue diff = PW_TIMESTAMP;

    diff.ts_seconds = a->ts_seconds - b->ts_seconds;
    diff.ts_nanoseconds = a->ts_nanoseconds - b->ts_nanoseconds;
    if (a->ts_nanoseconds < b->ts_nanoseconds) {
        diff.ts_seconds--;
        diff.ts_nanoseconds += 1000'000'000UL;
    }
    return diff;
}
