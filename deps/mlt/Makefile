#/**********************************************************************
# *
# * PostGIS - Spatial Types for PostgreSQL
# * http://postgis.net
# *
# * PostGIS is free software: you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation, either version 2 of the License, or
# * (at your option) any later version.
# *
# * PostGIS is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# *
# * You should have received a copy of the GNU General Public License
# * along with PostGIS.  If not, see <http://www.gnu.org/licenses/>.
# *
# **********************************************************************
# *
# * Copyright 2025 Dan Polevoy
# *
# **********************************************************************/

top_builddir = ../..
top_srcdir = ../..
srcdir = .
libdir = ${exec_prefix}/lib

CXX = clang -std=c++17 -x c++
CXXFLAGS = -std=c++20 \
	-I$(top_srcdir)/liblwgeom \
	-I$(top_builddir)/liblwgeom \
	-I$(srcdir)/vendor/mlt-include \
	-I$(srcdir)/vendor/mlt-src \
	-I$(srcdir)/vendor/mlt-vendor/fsst \
	-I$(srcdir)/vendor/mlt-vendor/fastpfor/headers \
	-I$(srcdir)/vendor/mlt-vendor/earcut/include \
	-I$(srcdir)/vendor/mlt-vendor/simde \
	 -I/opt/homebrew/Cellar/geos/3.14.1/include -I/opt/homebrew/Cellar/proj/9.7.0/include -I/opt/homebrew/opt/libtiff/include -I/opt/homebrew/opt/zstd/include -I/opt/homebrew/Cellar/xz/5.8.2/include -I/opt/homebrew/opt/jpeg-turbo/include -I/opt/homebrew/Cellar/protobuf-c/1.5.2_10/include -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include -I/opt/homebrew/Cellar/sfcgal/2.2.0_1/include -I/opt/homebrew/Cellar/json-c/0.18/include -I/opt/homebrew/Cellar/json-c/0.18/include/json-c -I/opt/homebrew/Cellar/pcre2/10.47/include -DNDEBUG   -fno-common -DPIC -O3 \
	-DMLT_ENABLE_FASTPFOR=1 -DSIMDE_ENABLE_NATIVE_ALIASES -w
LDFLAGS =  -lm



MLT_OBJS = \
	lwgeom_mlt.o \
	vendor/mlt-src/mlt/encoder.o \
	vendor/mlt-src/mlt/encode/int.o \
	vendor/mlt-src/mlt/metadata/stream.o \
	vendor/mlt-src/mlt/metadata/tileset.o \
	vendor/mlt-vendor/fsst/libfsst.o \
	vendor/mlt-vendor/fsst/fsst_avx512.o \
	vendor/mlt-vendor/fastpfor/src/bitpacking.o \
	vendor/mlt-vendor/fastpfor/src/bitpackingaligned.o \
	vendor/mlt-vendor/fastpfor/src/bitpackingunaligned.o \
	vendor/mlt-vendor/fastpfor/src/horizontalbitpacking.o \
	vendor/mlt-vendor/fastpfor/src/simdbitpacking.o \
	vendor/mlt-vendor/fastpfor/src/simdunalignedbitpacking.o

all: libmlt.la

libmlt.la: $(MLT_OBJS)
	ar rs libmlt.la $(MLT_OBJS)

lwgeom_mlt.o: lwgeom_mlt.cpp lwgeom_mlt.h ../../liblwgeom/liblwgeom.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

vendor/mlt-src/mlt/%.o: vendor/mlt-src/mlt/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

vendor/mlt-vendor/fsst/%.o: vendor/mlt-vendor/fsst/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -std=c++17 -c -o $@ $<

vendor/mlt-vendor/fastpfor/src/%.o: vendor/mlt-vendor/fastpfor/src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

../../liblwgeom/liblwgeom.h:
	$(MAKE) -C ../../liblwgeom liblwgeom.h

clean:
	rm -f libmlt.la $(MLT_OBJS)

distclean: clean
	rm -f Makefile

install:

uninstall:

check:

.PHONY: clean distclean install uninstall check
