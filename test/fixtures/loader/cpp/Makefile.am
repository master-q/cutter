INCLUDES =					\
	$(CUTTER_CFLAGS)			\
	-I$(top_builddir)			\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/cutter			\
	-I$(top_srcdir)/cppcutter

noinst_LTLIBRARIES =				\
	stub-namespace.la			\
	stub-multi-namespace.la			\
	stub-nested-namespace.la

AM_LDFLAGS =			\
	-module			\
	-rpath $(libdir)	\
	-avoid-version		\
	-no-undefined

LIBS =							\
	$(top_builddir)/cutter/libcutter.la		\
	$(top_builddir)/gcutter/libgcutter.la		\
	$(top_builddir)/cppcutter/libcppcutter.la	\
	$(GLIB_LIBS)					\
	$(GIO_LIBS)

stub_namespace_la_SOURCES	= stub-namespace.cpp
stub_multi_namespace_la_SOURCES	= stub-multi-namespace.cpp
stub_nested_namespace_la_SOURCES	= stub-nested-namespace.cpp

echo-tests:
	@echo $(noinst_LTLIBRARIES)
