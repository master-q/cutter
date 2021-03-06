/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2007-2011  Kouhei Sutou <kou@clear-code.com>
 *
 *  This library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __CUT_PUBLIC_H__
#define __CUT_PUBLIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <cutter/cut-config.h>

#include <stdarg.h>
#include <stdlib.h>
#include <setjmp.h>
#ifdef CUT_HAVE_INTTYPES_H
#  include <inttypes.h>
#elif defined(CUT_HAVE_STDINT_H)
#  include <stdint.h>
#endif

#ifndef CUT_DISABLE_SOCKET_SUPPORT
#  ifdef CUT_HAVE_WINSOCK2_H
#    include <winsock2.h>
#    include <ws2tcpip.h>
#  else
#    include <sys/socket.h>
#    ifdef CUT_HAVE_SYS_UN_H
#      include <sys/un.h>
#    endif
#  endif
#endif

#include <cutter/cut-types.h>
#include <cutter/cut-macros.h>
#include <cutter/cut-multi-process.h>

typedef struct _CutTestContext     CutTestContext;

typedef enum {
    CUT_TEST_RESULT_INVALID = -1,
    CUT_TEST_RESULT_SUCCESS,
    CUT_TEST_RESULT_NOTIFICATION,
    CUT_TEST_RESULT_OMISSION,
    CUT_TEST_RESULT_PENDING,
    CUT_TEST_RESULT_FAILURE,
    CUT_TEST_RESULT_ERROR,
    CUT_TEST_RESULT_CRASH,
    CUT_TEST_RESULT_LAST
} CutTestResultStatus;

void            cut_test_context_current_push(CutTestContext *context);
CutTestContext *cut_test_context_current_pop (void);
CutTestContext *cut_test_context_current_peek(void);

void  cut_test_context_keep_user_message    (CutTestContext *context);
void  cut_test_context_set_user_message     (CutTestContext *context,
                                             const char *format,
                                             ...) CUT_GNUC_PRINTF(2, 3);
void  cut_test_context_set_user_message_backward_compatibility
                                            (CutTestContext *context,
                                             const char *format,
                                             ...);
void  cut_test_context_set_user_message_va_list
                                            (CutTestContext *context,
                                             const char *format,
                                             va_list args);
void  cut_test_context_check_optional_assertion_message
                                            (CutTestContext *context);
void  cut_test_context_set_expected         (CutTestContext *context,
                                             const char *expected);
void  cut_test_context_set_actual           (CutTestContext *context,
                                             const char *actual);
void  cut_test_context_pass_assertion       (CutTestContext *context);
void  cut_test_context_set_current_result   (CutTestContext *context,
                                             CutTestResultStatus status,
                                             const char *system_message);
void  cut_test_context_set_current_result_user_message
                                            (CutTestContext *context,
                                             const char *user_message);
void  cut_test_context_process_current_result
                                            (CutTestContext *context);
cut_boolean
      cut_test_context_get_have_current_result
                                            (CutTestContext *context);
void  cut_test_context_register_result      (CutTestContext *context,
                                             CutTestResultStatus status,
                                             const char *system_message);
void         cut_test_context_start_user_message_jump
                                            (CutTestContext *context);
void         cut_test_context_finish_user_message_jump
                                            (CutTestContext *context);
cut_boolean  cut_test_context_in_user_message_jump
                                            (CutTestContext *context);
void         cut_test_context_set_jump      (CutTestContext *context,
                                             jmp_buf        *buffer);
jmp_buf     *cut_test_context_get_jump      (CutTestContext *context);
void         cut_test_context_long_jump     (CutTestContext *context);

const void *cut_test_context_take           (CutTestContext *context,
                                             void           *object,
                                             CutDestroyFunction destroy_function);
const void *cut_test_context_take_memory    (CutTestContext *context,
                                             void           *memory);
const char *cut_test_context_take_string    (CutTestContext *context,
                                             char           *string);
const char *cut_test_context_take_strdup    (CutTestContext *context,
                                             const char     *string);
const char *cut_test_context_take_strndup   (CutTestContext *context,
                                             const char     *string,
                                             size_t          size);
const void *cut_test_context_take_memdup    (CutTestContext *context,
                                             const void     *memory,
                                             size_t          size);
const char *cut_test_context_take_printf    (CutTestContext *context,
                                             const char     *format,
                                             ...) CUT_GNUC_PRINTF(2, 3);
const char **cut_test_context_take_string_array
                                            (CutTestContext *context,
                                             char          **strings);

char        *cut_utils_inspect_memory       (const void *memory,
                                             size_t      size);

cut_boolean  cut_utils_equal_string         (const char *string1,
                                             const char *string2);
cut_boolean  cut_utils_equal_substring      (const char *string1,
                                             const char *string2,
                                             size_t      length);
cut_boolean  cut_utils_equal_double         (double double1,
                                             double double2,
                                             double error);
cut_boolean  cut_utils_equal_string_array   (char **strings1,
                                             char **strings2);
char        *cut_utils_inspect_string_array (char **strings);
char        *cut_utils_inspect_string       (const char *string);

#ifndef CUT_DISABLE_SOCKET_SUPPORT
cut_boolean  cut_utils_equal_sockaddr       (const struct sockaddr *address1,
                                             const struct sockaddr *address2);
char        *cut_utils_inspect_sockaddr     (const struct sockaddr *address);
#endif

cut_boolean  cut_utils_path_exist           (const char *path);
char        *cut_utils_build_path           (const char *path,
                                             ...) CUT_GNUC_NULL_TERMINATED;
char        *cut_utils_build_path_va_list   (const char *path,
                                             va_list args);
char        *cut_utils_build_path_array     (const char **paths);
void         cut_utils_remove_path_recursive_force
                                            (const char *path);
void         cut_utils_make_directory_recursive_force
                                            (const char *path);
cut_boolean  cut_utils_regex_match          (const char *pattern,
                                             const char *string);
char        *cut_utils_fold                 (const char *string);
char        *cut_utils_append_diff          (const char *message,
                                             const char *from,
                                             const char *to);

const char *cut_test_context_build_fixture_path
                                            (CutTestContext *context,
                                             const char     *path,
                                             ...) CUT_GNUC_NULL_TERMINATED;

const char *cut_utils_get_fixture_data      (CutTestContext *context,
                                             const char **fixture_path,
                                             size_t *size,
                                             const char *path,
                                             ...) CUT_GNUC_NULL_TERMINATED;
const char *cut_utils_get_fixture_data_va_list
                                            (CutTestContext *context,
                                             const char **fixture_path,
                                             size_t *size,
                                             const char *path,
                                             va_list args);

void  cut_test_context_add_data             (CutTestContext *context,
                                             const char     *first_data_name,
                                             ...) CUT_GNUC_NULL_TERMINATED;

void  cut_test_context_set_attributes       (CutTestContext *context,
                                             const char     *first_attribute_name,
                                             ...) CUT_GNUC_NULL_TERMINATED;

int   cut_test_context_trap_fork            (CutTestContext *context);
int   cut_test_context_wait_process         (CutTestContext *context,
                                             int             pid,
                                             unsigned int    usec_timeout);
const char *cut_test_context_get_forked_stdout_message
                                            (CutTestContext *context,
                                             int             pid);
const char *cut_test_context_get_forked_stderr_message
                                            (CutTestContext *context,
                                             int             pid);

void  cut_test_context_set_fixture_data_dir (CutTestContext *context,
                                             const char     *path,
                                             ...) CUT_GNUC_NULL_TERMINATED;

void        cut_test_context_push_backtrace (CutTestContext *context,
                                             const char     *relative_path,
                                             const char     *file_name,
                                             unsigned int    line,
                                             const char     *function_name,
                                             const char     *info);
void        cut_test_context_pop_backtrace  (CutTestContext *context);
void        cut_test_context_get_last_backtrace
                                            (CutTestContext *context,
                                             const char    **filename,
                                             unsigned int   *line,
                                             const char    **function_name,
                                             const char    **info);

char       *cut_diff_readable               (const char     *from,
                                             const char     *to);
char       *cut_diff_readable_folded        (const char     *from,
                                             const char     *to);
char       *cut_diff_unified                (const char     *from,
                                             const char     *to,
                                             const char     *from_label,
                                             const char     *to_label);

CutSubProcess *cut_utils_take_new_sub_process     (const char     *test_directory,
                                                   CutTestContext *test_context);

CutSubProcessGroup *cut_utils_take_new_sub_process_group (CutTestContext *test_context);

#ifdef __cplusplus
}
#endif

#endif /* __CUT_PUBLIC_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
