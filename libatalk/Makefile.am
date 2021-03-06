
# Makefile.am for libatalk/

SUBDIRS = acl adouble bstring cnid dsi util unicode vfs

lib_LTLIBRARIES = libatalk.la

libatalk_la_SOURCES = dummy.c

libatalk_la_LIBADD  = \
	acl/libacl.la \
	adouble/libadouble.la	\
	bstring/libbstring.la \
	dsi/libdsi.la		\
	util/libutil.la		\
	unicode/libunicode.la \
	vfs/libvfs.la

libatalk_la_DEPENDENCIES = \
	acl/libacl.la \
	adouble/libadouble.la	\
	bstring/libbstring.la \
	dsi/libdsi.la		\
	util/libutil.la		\
	unicode/libunicode.la \
	vfs/libvfs.la

libatalk_la_LDFLAGS = -static
