plugin_includedir = $(includedir)/skivvy
plugin_librarydir = $(libdir)/skivvy
plugin_manualsdir = $(mandir)/skivvy

DEF_FLAGS = \
	-D STAMP=\"$(shell date +%Y%m%d-%H%M%S)\" \
	-D DEV=\"$(shell git diff --quiet || echo -dev)\" \
	-D COMMITS=\"$(shell printf %04d \"$(git log --after={yesterday} --pretty=oneline|wc -l)\")\" \
	-D REVISION=\"$(shell git log -n 1 --pretty=format:%h|tr [:lower:] [:upper:])\" \
	-D DEFAULT_PLUGIN_DIR=\"$(plugindir)\"

GEN_FLAGS = -Wl,-E -Wfatal-errors -Wall -Wextra -Winit-self -ansi -pedantic -pipe -pthread \
	-I$(top_srcdir)/src/include
	
AM_CPPFLAGS = $(SOOKEE_CFLAGS) $(SKIVVY_CFLAGS)
AM_CXXFLAGS = $(GEN_FLAGS) $(DEF_FLAGS)

# -Wl,-E is required for programs that load plugin's
PLUGIN_FLAGS = -Wl,-E

plugin_include_HEADERS = \
	$(srcdir)/include/skivvy/plugin-pfinder.h \
	$(srcdir)/include/skivvy/plugin-pfinder-oacom.h

plugin_library_LTLIBRARIES =  \
	skivvy-plugin-pfinder.la

# IrcBot plugins
skivvy_plugin_pfinder_la_SOURCES = plugin-pfinder.cpp plugin-pfinder-oacom.cpp
skivvy_plugin_pfinder_la_LDFLAGS = -module -avoid-version $(PLUGIN_FLAGS)
skivvy_plugin_pfinder_la_LIBADD =  $(SOOKEE_LIBS) $(SKIVVY_LIBS) -L.libs

plugin_manuals_DATA = $(top_srcdir)/docs/manual.text

extra_DIST = $(top_srcdir)/docs/manual.text

install-data-hook: $(plugin_library_LTLIBRARIES)
	for m in $(plugin_library_LTLIBRARIES); do \
		base=`echo $$m | sed 's/\..*//'`; \
		rm -f $(plugin_librarydir)/$$base.la; \
	done
