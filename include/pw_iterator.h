#pragma once

#include <pw_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /*
     * Arguments for Iterator constructor
     */
    PwValuePtr iterable;
} PwIteratorCtorArgs;

/*
 * Shorthand methods
 */
[[nodiscard]] static inline bool     pw_start_read_lines (PwValuePtr reader) { return pw_interface(reader->type_id, LineReader)->start(reader); }
[[nodiscard]] static inline bool     pw_read_line        (PwValuePtr reader, PwValuePtr result) { return pw_interface(reader->type_id, LineReader)->read_line(reader, result); }
[[nodiscard]] static inline bool     pw_read_line_inplace(PwValuePtr reader, PwValuePtr line)   { return pw_interface(reader->type_id, LineReader)->read_line_inplace(reader, line); }
[[nodiscard]] static inline bool     pw_unread_line      (PwValuePtr reader, PwValuePtr line)   { return pw_interface(reader->type_id, LineReader)->unread_line(reader, line); }
[[nodiscard]] static inline unsigned pw_get_line_number  (PwValuePtr reader) { return pw_interface(reader->type_id, LineReader)->get_line_number(reader); }
              static inline void     pw_stop_read_lines  (PwValuePtr reader) { pw_interface(reader->type_id, LineReader)->stop(reader); }


#ifdef __cplusplus
}
#endif
