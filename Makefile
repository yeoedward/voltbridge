LIBVOLTDB = libvoltdb.so
MODULE_big = nstore
OBJS = nstore_hook.o voltdb_bridge.o
CXXFLAGS += -fpic -Ivoltdb/src/ee -Ivoltdb/third_party/cpp \
	-Wall -Wextra -Werror -Woverloaded-virtual \
	-Wpointer-arith -Wcast-qual -Wwrite-strings \
	-Winit-self -Wno-sign-compare -Wno-unused-parameter \
	-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DNOCLOCK \
	-fno-omit-frame-pointer \
	-fvisibility=default \
	-DBOOST_SP_DISABLE_THREADS -DBOOST_DISABLE_THREADS -DBOOST_ALL_NO_LIB \
	-Wno-ignored-qualifiers
SHLIB_LINK = -lstdc++ -L. -lvoltdb
EXTRA_CLEAN = $(LIBVOLTDB)

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/nstore
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

install: cp_voltdb_so

cp_voltdb_so:
	cp $(LIBVOLTDB) $(top_builddir)/install/lib/

voltdb_bridge.o: hardcoded.h $(LIBVOLTDB)

$(LIBVOLTDB):
	ant -buildfile voltdb/build.xml ee
	cp voltdb/voltdb/libvoltdb-4.7.so $(LIBVOLTDB)
