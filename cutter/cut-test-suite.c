/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2007-2009  Kouhei Sutou <kou@cozmixng.org>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <glib.h>
#include <setjmp.h>
#include <signal.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "cut-test-suite.h"

#include "cut-test.h"
#include "cut-test-case.h"
#include "cut-run-context.h"
#include "cut-main.h"
#include "cut-utils.h"
#include "cut-test-result.h"
#include "cut-backtrace-entry.h"
#include "cut-crash-backtrace.h"

#include "../gcutter/gcut-marshalers.h"
#include "../gcutter/gcut-error.h"

#define CUT_TEST_SUITE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CUT_TYPE_TEST_SUITE, CutTestSuitePrivate))

typedef struct _CutTestSuitePrivate	CutTestSuitePrivate;
struct _CutTestSuitePrivate
{
    CutWarmupFunction warmup;
    CutCooldownFunction cooldown;
};

enum
{
    PROP_0,
    PROP_WARMUP_FUNCTION,
    PROP_COOLDOWN_FUNCTION
};

enum
{
    READY,
    START_TEST_CASE,
    COMPLETE_TEST_CASE,
    LAST_SIGNAL
};

static gint cut_test_suite_signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE (CutTestSuite, cut_test_suite, CUT_TYPE_TEST_CONTAINER)

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void
cut_test_suite_class_init (CutTestSuiteClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_pointer("warmup-function",
                                "Warmup Function",
                                "The function for warmup",
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_WARMUP_FUNCTION, spec);

    spec = g_param_spec_pointer("cooldown-function",
                                "Cooldown Function",
                                "The function for cooldown",
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_COOLDOWN_FUNCTION, spec);

    cut_test_suite_signals[READY]
        = g_signal_new("ready",
                       G_TYPE_FROM_CLASS(klass),
                       G_SIGNAL_RUN_LAST,
                       G_STRUCT_OFFSET(CutTestSuiteClass, ready),
                       NULL, NULL,
                       _gcut_marshal_VOID__UINT_UINT,
                       G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);

    cut_test_suite_signals[START_TEST_CASE]
        = g_signal_new("start-test-case",
                       G_TYPE_FROM_CLASS(klass),
                       G_SIGNAL_RUN_LAST,
                       G_STRUCT_OFFSET(CutTestSuiteClass, start_test_case),
                       NULL, NULL,
                       g_cclosure_marshal_VOID__OBJECT,
                       G_TYPE_NONE, 1, CUT_TYPE_TEST_CASE);

    cut_test_suite_signals[COMPLETE_TEST_CASE]
        = g_signal_new("complete-test-case",
                       G_TYPE_FROM_CLASS(klass),
                       G_SIGNAL_RUN_LAST,
                       G_STRUCT_OFFSET(CutTestSuiteClass, complete_test_case),
                       NULL, NULL,
                       _gcut_marshal_VOID__OBJECT_BOOLEAN,
                       G_TYPE_NONE, 2, CUT_TYPE_TEST_CASE, G_TYPE_BOOLEAN);

    g_type_class_add_private(gobject_class, sizeof(CutTestSuitePrivate));
}

static void
cut_test_suite_init (CutTestSuite *test_suite)
{
    CutTestSuitePrivate *priv = CUT_TEST_SUITE_GET_PRIVATE(test_suite);

    priv->warmup = NULL;
    priv->cooldown = NULL;
}

static void
dispose (GObject *object)
{
    G_OBJECT_CLASS(cut_test_suite_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    CutTestSuitePrivate *priv = CUT_TEST_SUITE_GET_PRIVATE(object);

    switch (prop_id) {
      case PROP_WARMUP_FUNCTION:
        priv->warmup = g_value_get_pointer(value);
        break;
      case PROP_COOLDOWN_FUNCTION:
        priv->cooldown = g_value_get_pointer(value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    CutTestSuitePrivate *priv = CUT_TEST_SUITE_GET_PRIVATE(object);

    switch (prop_id) {
      case PROP_WARMUP_FUNCTION:
        g_value_set_pointer(value, priv->warmup);
        break;
      case PROP_COOLDOWN_FUNCTION:
        g_value_set_pointer(value, priv->cooldown);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

CutTestSuite *
cut_test_suite_new (const gchar *name,
                    CutWarmupFunction warmup, CutCooldownFunction cooldown)
{
    return g_object_new(CUT_TYPE_TEST_SUITE,
                        "element-name", "test-suite",
                        "name", name,
                        "warmup-function", warmup,
                        "cooldown-function", cooldown,
                        NULL);
}

CutTestSuite *
cut_test_suite_new_empty (void)
{
    return cut_test_suite_new(NULL, NULL, NULL);
}

typedef struct _RunTestInfo
{
    CutTestSuite *test_suite;
    CutTestCase *test_case;
    CutRunContext *run_context;
    gchar **test_names;
} RunTestInfo;

static void
run (gpointer data, gpointer user_data)
{
    RunTestInfo *info = data;
    CutTestSuite *test_suite;
    CutTestCase *test_case;
    CutRunContext *run_context;
    gchar **test_names;
    gboolean *success = user_data;

    test_suite = info->test_suite;
    test_case = info->test_case;
    run_context = info->run_context;
    test_names = info->test_names;

    g_signal_emit_by_name(test_suite, "start-test-case", test_case);
    if (!cut_test_case_run_with_filter(test_case, run_context,
                                       (const gchar**)test_names))
        *success =  FALSE;
    g_signal_emit_by_name(test_suite, "complete-test-case", test_case, *success);

    g_object_unref(test_suite);
    g_object_unref(test_case);
    g_object_unref(run_context);
    g_strfreev(test_names);
    g_free(info);
}

static void
run_with_thread_support (CutTestSuite *test_suite, CutTestCase *test_case,
                         CutRunContext *run_context, const gchar **test_names,
                         GThreadPool *thread_pool, gboolean *success)
{
    RunTestInfo *info;
    gboolean need_no_thread_run = TRUE;

    if (cut_run_context_is_canceled(run_context))
        return;

    info = g_new0(RunTestInfo, 1);
    info->test_suite = g_object_ref(test_suite);
    info->test_case = g_object_ref(test_case);
    info->run_context = g_object_ref(run_context);
    info->test_names = g_strdupv((gchar **)test_names);

    if (thread_pool) {
        GError *error = NULL;

        g_thread_pool_push(thread_pool, info, &error);
        if (error) {
            gchar *inspected;

            inspected = gcut_error_inspect(error);
            g_warning("%s", inspected);
            g_free(inspected);
            g_error_free(error);
        } else {
            need_no_thread_run = FALSE;
        }
    }

    if (need_no_thread_run)
        run(info, success);
}

static void
emit_ready_signal (CutTestSuite *test_suite, GList *test_cases,
                   CutRunContext *run_context)
{
    GList *node;
    guint n_test_cases, n_tests;

    n_test_cases = 0;
    n_tests = 0;

    for (node = test_cases; node; node = g_list_next(node)) {
        CutTestCase *test_case = node->data;

        n_test_cases++;
        n_tests += cut_test_container_get_n_tests(CUT_TEST_CONTAINER(test_case),
                                                  run_context);
    }

    g_signal_emit_by_name(test_suite, "ready", n_test_cases, n_tests);
}

static gboolean
cut_test_suite_run_test_cases (CutTestSuite *test_suite,
                               CutRunContext *run_context,
                               GList *test_cases, const gchar **test_names)
{
    CutTestSuitePrivate *priv;
    GList *node;
    GThreadPool *thread_pool = NULL;
    GList *sorted_test_cases;
    gboolean try_thread;
    gboolean all_success = TRUE;
    gint signum;
    jmp_buf jump_buffer;
    CutCrashBacktrace *crash_backtrace = NULL;

    priv = CUT_TEST_SUITE_GET_PRIVATE(test_suite);

    sorted_test_cases = g_list_copy(test_cases);
    sorted_test_cases = cut_run_context_sort_test_cases(run_context,
                                                        sorted_test_cases);

    try_thread = cut_run_context_get_multi_thread(run_context);
    if (try_thread) {
        GError *error = NULL;
        gint max_threads;

        max_threads = cut_run_context_get_max_threads(run_context);
        thread_pool = g_thread_pool_new(run, &all_success,
                                        max_threads, FALSE, &error);
        if (error) {
            cut_utils_report_error(error);
        }
    }

    if (cut_run_context_get_handle_signals(run_context)) {
        crash_backtrace = cut_crash_backtrace_new(&jump_buffer);
        signum = setjmp(jump_buffer);
    } else {
        signum = 0;
    }
    switch (signum) {
    case 0:
        emit_ready_signal(test_suite, sorted_test_cases, run_context);
        g_signal_emit_by_name(test_suite, "start", NULL);

        if (priv->warmup)
            priv->warmup();

        for (node = sorted_test_cases; node; node = g_list_next(node)) {
            CutTestCase *test_case = node->data;

            if (!test_case)
                continue;
            if (CUT_IS_TEST_CASE(test_case)) {
                run_with_thread_support(test_suite, test_case, run_context,
                                        test_names, thread_pool, &all_success);
            } else {
                g_warning("This object is not test case!");
            }
        }

        if (thread_pool)
            g_thread_pool_free(thread_pool, FALSE, TRUE);

        if (all_success) {
            CutTestResult *result;
            result = cut_test_result_new(CUT_TEST_RESULT_SUCCESS,
                                         NULL, NULL, NULL, test_suite, NULL,
                                         NULL, NULL, NULL);
            cut_test_emit_result_signal(CUT_TEST(test_suite), NULL, result);
            g_object_unref(result);
        }
        if (crash_backtrace)
            cut_crash_backtrace_free(crash_backtrace);
        break;
#ifndef G_OS_WIN32
    case SIGSEGV:
    case SIGABRT:
    case SIGTERM:
        all_success = FALSE;
        cut_crash_backtrace_emit(test_suite, NULL, NULL, NULL, NULL, NULL);
        break;
    case SIGINT:
        cut_run_context_cancel(run_context);
        break;
#endif
    default:
        break;
    }

    if (priv->cooldown)
        priv->cooldown();

    g_signal_emit_by_name(CUT_TEST(test_suite), "complete", NULL, all_success);

    g_list_free(sorted_test_cases);

    return all_success;
}

gboolean
cut_test_suite_run (CutTestSuite *suite, CutRunContext *run_context)
{
    const gchar **test_case_names;
    const gchar **test_names;

    test_case_names = cut_run_context_get_target_test_case_names(run_context);
    test_names = cut_run_context_get_target_test_names(run_context);
    return cut_test_suite_run_with_filter(suite, run_context,
                                          test_case_names,
                                          test_names);
}

gboolean
cut_test_suite_run_test_case (CutTestSuite *suite, CutRunContext *run_context,
                              gchar *test_case_name)
{
    gchar *test_case_names[] = {NULL, NULL};

    g_return_val_if_fail(CUT_IS_TEST_SUITE(suite), FALSE);

    test_case_names[0] = test_case_name;
    return cut_test_suite_run_with_filter(suite, run_context,
                                          (const gchar **)test_case_names, NULL);
}

gboolean
cut_test_suite_run_test (CutTestSuite *suite, CutRunContext *run_context,
                         gchar *test_name)
{
    GList *test_cases;
    const gchar *test_names[] = {NULL, NULL};

    g_return_val_if_fail(CUT_IS_TEST_SUITE(suite), FALSE);

    test_names[0] = test_name;
    test_cases = cut_test_container_get_children(CUT_TEST_CONTAINER(suite));
    return cut_test_suite_run_test_cases(suite, run_context,
                                         test_cases, test_names);
}

gboolean
cut_test_suite_run_test_in_test_case (CutTestSuite *suite,
                                      CutRunContext *run_context,
                                      gchar        *test_name,
                                      gchar        *test_case_name)
{
    gchar *test_names[] = {NULL, NULL};
    gchar *test_case_names[] = {NULL, NULL};

    g_return_val_if_fail(CUT_IS_TEST_SUITE(suite), FALSE);

    test_names[0] = test_name;
    test_case_names[0] = test_case_name;

    return cut_test_suite_run_with_filter(suite, run_context,
                                          (const gchar **)test_case_names,
                                          (const gchar **)test_names);
}

gboolean
cut_test_suite_run_with_filter (CutTestSuite *test_suite,
                                CutRunContext *run_context,
                                const gchar **test_case_names,
                                const gchar **test_names)
{
    CutTestContainer *container;
    GList *filtered_test_cases = NULL;
    gboolean success = TRUE;

    container = CUT_TEST_CONTAINER(test_suite);
    if (test_case_names && *test_case_names) {
        filtered_test_cases =
            cut_test_container_filter_children(container, test_case_names);
    } else {
        CutTestContainer *container;

        container = CUT_TEST_CONTAINER(test_suite);
        filtered_test_cases =
            g_list_copy(cut_test_container_get_children(container));
    }

    if (!filtered_test_cases)
        return success;

    success = cut_test_suite_run_test_cases(test_suite, run_context,
                                            filtered_test_cases, test_names);
    g_list_free(filtered_test_cases);

    return success;
}


void
cut_test_suite_add_test_case (CutTestSuite *suite, CutTestCase *test_case)
{
    g_return_if_fail(CUT_IS_TEST_CASE(test_case));
    g_return_if_fail(CUT_IS_TEST_SUITE(suite));

    cut_test_container_add_test(CUT_TEST_CONTAINER(suite), CUT_TEST(test_case));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
