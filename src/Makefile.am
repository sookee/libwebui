# /*-----------------------------------------------------------------.
# | Copyright (C) 2015 SooKee oasookee@googlemail.com                |
# '------------------------------------------------------------------'
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
# 
# http://www.gnu.org/licenses/gpl-2.0.html
# 
# '-----------------------------------------------------------------*/

webui_includedir = $(prefix)/include/webui
webui_librarydir = $(libdir)/webui
webui_manualsdir = $(datadir)/webui

INC_SOOKEE = -I$(prefix)/include
LIB_SOOKEE = -L$(libdir)/sookee -lsookee

DEF_FLAGS = \
	-D STAMP=\"$(shell date +%Y%m%d-%H%M%S)\" \
	-D DEV=\"$(shell git diff --quiet || echo -dev)\" \
	-D COMMITS=\"$(shell printf %04d \"$(git log --after={yesterday} --pretty=oneline|wc -l)\")\" \
	-D REVISION=\"$(shell git log -n 1 --pretty=format:%h|tr [:lower:] [:upper:])\"

AM_CPPFLAGS = $(INC_SOOKEE)
AM_CXXFLAGS = -std=gnu++14 $(DEF_FLAGS) -I../../src/include/webui

webui_include_HEADERS = \
	$(srcdir)/include/webui/webui.h
	
webui_library_LTLIBRARIES = \
	libwebui.la

# webui framework
libwebui_la_SOURCES = \
	webui.cpp
	
#libwebui_la_CXXFLAGS = -I./include
libwebui_la_LIBADD = -L.libs $(LIB_SOOKEE)
libwebui_la_LDFLAGS = -version-info 0:0:0

noinst_PROGRAMS = test

test_SOURCES = test.cpp
test_LDADD = -lwebui $(LIB_SOOKEE)

webui_manuals_DATA = $(top_srcdir)/docs/webui-manual.text

extra_DIST = $(top_srcdir)/docs/manual.text
