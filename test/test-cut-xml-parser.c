#include "cutter.h"
#include "cut-xml-parser.h"

void test_xml (void);

void
test_xml (void)
{
    CutTestResult *result;
    const gchar xml[] = "<result>\n"
                        "  <test-case>\n"
                        "    <name>dummy test case</name>\n"
                        "  </test-case>\n"
                        "  <test>\n"
                        "    <name>dummy-error-test</name>\n"
                        "  </test>\n"
                        "  <status>error</status>\n"
                        "  <detail>This test should error</detail>\n"
                        "  <backtrace>\n"
                        "    <file>test-cut-report-xml.c</file>\n"
                        "    <line>31</line>\n"
                        "    <info>dummy_error_test()</info>\n"
                        "  </backtrace>\n"
                        "  <elapsed>0.000100</elapsed>\n"
                        "</result>\n";
    result = cut_xml_parse_test_result_xml(xml, -1);

    cut_assert(result);
    cut_assert_equal_string("dummy test case",
                            cut_test_result_get_test_case_name(result));
    cut_assert_equal_string("dummy-error-test",
                            cut_test_result_get_test_name(result));
    cut_assert_equal_int(CUT_TEST_RESULT_ERROR,
                         cut_test_result_get_status(result));
    cut_assert_equal_double(0.0001, 0.0, cut_test_result_get_elapsed(result));
    cut_assert_equal_int(31, cut_test_result_get_line(result));
    cut_assert_equal_string("test-cut-report-xml.c",
                            cut_test_result_get_filename(result));
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
