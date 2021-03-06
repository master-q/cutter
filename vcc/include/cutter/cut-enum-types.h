


#ifndef __CUT_ENUM_TYPES_H__
#define __CUT_ENUM_TYPES_H__

#include <glib-object.h>

G_BEGIN_DECLS
/* enumerations from "cut-public.h" */
GType cut_test_result_status_get_type (void);
#define CUT_TYPE_TEST_RESULT_STATUS (cut_test_result_status_get_type())
GType cut_diff_writer_tag_get_type (void);
#define CUT_TYPE_DIFF_WRITER_TAG (cut_diff_writer_tag_get_type())
GType cut_file_stream_reader_error_get_type (void);
#define CUT_TYPE_FILE_STREAM_READER_ERROR (cut_file_stream_reader_error_get_type())
GType cut_pipeline_error_get_type (void);
#define CUT_TYPE_PIPELINE_ERROR (cut_pipeline_error_get_type())
GType cut_order_get_type (void);
#define CUT_TYPE_ORDER (cut_order_get_type())
GType cut_stream_reader_error_get_type (void);
#define CUT_TYPE_STREAM_READER_ERROR (cut_stream_reader_error_get_type())
GType cut_test_context_error_get_type (void);
#define CUT_TYPE_TEST_CONTEXT_ERROR (cut_test_context_error_get_type())
GType cut_verbose_level_get_type (void);
#define CUT_TYPE_VERBOSE_LEVEL (cut_verbose_level_get_type())
GType cut_verbose_level_error_get_type (void);
#define CUT_TYPE_VERBOSE_LEVEL_ERROR (cut_verbose_level_error_get_type())
G_END_DECLS

#endif /* __CUT_ENUM_TYPES_H__ */



