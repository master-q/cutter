/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008-2011  Kouhei Sutou <kou@clear-code.com>
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

#include <glib.h>

#include "cut-stream-reader.h"
#include "cut-runner.h"
#include "cut-stream-parser.h"
#include <gcutter/gcut-io.h>

#define CUT_STREAM_READER_GET_PRIVATE(obj)                         \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CUT_TYPE_STREAM_READER,    \
                                 CutStreamReaderPrivate))

typedef struct _CutStreamReaderPrivate	CutStreamReaderPrivate;
struct _CutStreamReaderPrivate
{
    CutStreamParser *parser;
    gboolean error_emitted;
    guint read_source_id;
    guint hung_up_source_id;
    guint error_source_id;
    gboolean ended;
};

G_DEFINE_TYPE(CutStreamReader, cut_stream_reader, CUT_TYPE_RUN_CONTEXT)

static void     dispose          (GObject         *object);

static void
cut_stream_reader_class_init (CutStreamReaderClass *klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;

    g_type_class_add_private(gobject_class, sizeof(CutStreamReaderPrivate));
}

static void
cut_stream_reader_init (CutStreamReader *stream_reader)
{
    CutStreamReaderPrivate *priv;

    priv = CUT_STREAM_READER_GET_PRIVATE(stream_reader);

    priv->parser = cut_stream_parser_new(CUT_RUN_CONTEXT(stream_reader));
    priv->error_emitted = FALSE;
    priv->read_source_id = 0;
    priv->hung_up_source_id = 0;
    priv->error_source_id = 0;
    priv->ended = FALSE;
}

static void
dispose_source (CutStreamReaderPrivate *priv)
{
    if (priv->read_source_id > 0) {
        g_source_remove(priv->read_source_id);
        priv->read_source_id = 0;
    }

    if (priv->hung_up_source_id > 0) {
        g_source_remove(priv->hung_up_source_id);
        priv->hung_up_source_id = 0;
    }

    if (priv->error_source_id > 0) {
        g_source_remove(priv->error_source_id);
        priv->error_source_id = 0;
    }
}

static void
dispose (GObject *object)
{
    CutStreamReaderPrivate *priv;

    priv = CUT_STREAM_READER_GET_PRIVATE(object);

    dispose_source(priv);

    if (priv->parser) {
        CutStreamParser *parser;

        parser = priv->parser;
        priv->parser = NULL;
        g_object_unref(parser);
    }

    G_OBJECT_CLASS(cut_stream_reader_parent_class)->dispose(object);
}

CutRunContext *
cut_stream_reader_new (void)
{
    return g_object_new(CUT_TYPE_STREAM_READER, NULL);
}

GQuark
cut_stream_reader_error_quark (void)
{
    return g_quark_from_static_string("cut-stream-reader-error-quark");
}

#define emit_error(stream_reader, code, sub_error, ...) do              \
{                                                                       \
    CutStreamReader *_stream_reader;                                    \
    CutStreamReaderPrivate *_priv;                                      \
    CutRunContext *_run_context;                                        \
                                                                        \
    _stream_reader = (stream_reader);                                   \
    _priv = CUT_STREAM_READER_GET_PRIVATE(_stream_reader);              \
    _run_context = CUT_RUN_CONTEXT(_stream_reader);                     \
    cut_run_context_emit_error(_run_context,                            \
                               CUT_STREAM_READER_ERROR,                 \
                               code, sub_error,                         \
                               __VA_ARGS__);                            \
    _priv->error_emitted = TRUE;                                        \
    cut_run_context_emit_complete_run(_run_context, FALSE);             \
} while (0)

static gboolean
read_channel (GIOChannel *channel, GIOCondition condition, gpointer data)
{
    CutRunContext *run_context;
    CutStreamReader *stream_reader = data;
    CutStreamReaderPrivate *priv;
    gboolean keep_callback = TRUE;

    priv = CUT_STREAM_READER_GET_PRIVATE(stream_reader);
    keep_callback = cut_stream_reader_read_from_io_channel(stream_reader,
                                                           channel);
    run_context = CUT_RUN_CONTEXT(stream_reader);
    if (cut_run_context_is_canceled(run_context))
        cut_run_context_emit_complete_run(run_context, TRUE);
    if (cut_run_context_is_completed(run_context))
        keep_callback = FALSE;

    if (!keep_callback)
        dispose_source(priv);

    return keep_callback;
}

static gboolean
hung_up_channel (GIOChannel *channel, GIOCondition condition, gpointer data)
{
    CutStreamReader *stream_reader = data;
    CutStreamReaderPrivate *priv;
    gboolean keep_callback = FALSE;

    cut_stream_reader_read_from_io_channel_to_end(stream_reader, channel);
    priv = CUT_STREAM_READER_GET_PRIVATE(stream_reader);
    dispose_source(priv);

    return keep_callback;
}

static gboolean
error_channel (GIOChannel *channel, GIOCondition condition, gpointer data)
{
    CutStreamReader *stream_reader = data;
    CutStreamReaderPrivate *priv;
    gboolean keep_callback = FALSE;
    gchar *message;

    message = gcut_io_inspect_condition(condition);
    emit_error(stream_reader,
               CUT_STREAM_READER_ERROR_IO_ERROR, NULL,
               "%s", message);
    g_free(message);

    priv = CUT_STREAM_READER_GET_PRIVATE(stream_reader);
    dispose_source(priv);

    return keep_callback;
}

void
cut_stream_reader_watch_io_channel (CutStreamReader *stream_reader,
                                    GIOChannel      *channel)
{
    CutStreamReaderPrivate *priv;

    priv = CUT_STREAM_READER_GET_PRIVATE(stream_reader);
    dispose_source(priv);

    priv->read_source_id = g_io_add_watch_full(channel,
                                               G_PRIORITY_LOW,
                                               G_IO_IN | G_IO_PRI,
                                               read_channel, stream_reader,
                                               NULL);
    priv->hung_up_source_id = g_io_add_watch_full(channel,
                                                  G_PRIORITY_LOW,
                                                  G_IO_HUP,
                                                  hung_up_channel,
                                                  stream_reader,
                                                  NULL);
    priv->error_source_id = g_io_add_watch_full(channel,
                                                G_PRIORITY_LOW,
                                                G_IO_ERR | G_IO_NVAL,
                                                error_channel, stream_reader,
                                                NULL);
}

#define BUFFER_SIZE 16384
gboolean
cut_stream_reader_read_from_io_channel (CutStreamReader *stream_reader,
                                        GIOChannel      *channel)
{
    CutStreamReaderPrivate *priv;
    GIOStatus status;
    gboolean eof = FALSE;
    gchar stream[BUFFER_SIZE];
    gsize length = 0;
    GError *error = NULL;

    priv = CUT_STREAM_READER_GET_PRIVATE(stream_reader);
    if (priv->error_emitted)
        return FALSE;

    status = g_io_channel_read_chars(channel, stream, BUFFER_SIZE,
                                     &length, &error);
    if (status == G_IO_STATUS_EOF)
        eof = TRUE;

    if (error) {
        emit_error(stream_reader,
                   CUT_STREAM_READER_ERROR_READ,
                   error, "failed to read stream");
        return FALSE;
    }

    if (length > 0) {
        if (!cut_stream_reader_read(stream_reader, stream, length))
            return FALSE;
    }
    if (eof)
        cut_stream_reader_end_read(stream_reader);

    return !priv->error_emitted;
}

gboolean
cut_stream_reader_read_from_io_channel_to_end (CutStreamReader *stream_reader,
                                               GIOChannel      *channel)
{
    CutStreamReaderPrivate *priv;
    gboolean succeeded = TRUE;

    priv = CUT_STREAM_READER_GET_PRIVATE(stream_reader);
    while (!priv->ended) {
        succeeded = cut_stream_reader_read_from_io_channel(stream_reader,
                                                           channel);
        if (!succeeded) {
            break;
        }
    }

    return succeeded;
}

gboolean
cut_stream_reader_read (CutStreamReader *stream_reader,
                        const gchar *stream, gsize length)
{
    CutStreamReaderPrivate *priv;
    GError *error = NULL;

    priv = CUT_STREAM_READER_GET_PRIVATE(stream_reader);
    cut_stream_parser_parse(priv->parser, stream, length, &error);
    if (error)
        emit_error(stream_reader,
                   CUT_STREAM_READER_ERROR_PARSE,
                   error, "failed to parse stream");

    return !priv->error_emitted;
}

gboolean
cut_stream_reader_end_read (CutStreamReader *stream_reader)
{
    CutStreamReaderPrivate *priv;
    GError *error = NULL;

    priv = CUT_STREAM_READER_GET_PRIVATE(stream_reader);
    if (priv->error_emitted)
        return FALSE;

    cut_stream_parser_end_parse(priv->parser, &error);
    if (error) {
        emit_error(stream_reader,
                   CUT_STREAM_READER_ERROR_END_PARSE,
                   error, "failed to end parse stream");
    } else {
        priv->ended = TRUE;
    }

    return !priv->error_emitted;
}

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
