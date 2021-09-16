ACLOCAL_AMFLAGS = -I m4

#cf_docdir = $(docdir)/geany-code-format
#cf_doc_DATA = README.md

#cf_datadir = $(pkgdatadir)
#cf_data_DATA = code-format.conf

geany_plugindir = $(libdir)/geany
geany_plugin_LTLIBRARIES = preview.la

preview_la_CFLAGS = $(GEANY_CFLAGS) $(WEBKIT_CFLAGS) $(CMARK_CFLAGS) \
	-DG_LOG_DOMAIN=\""CodeFormat"\" \
	-DFMT_README_FILE=\""$(cf_docdir)/README.md"\" \
	-DFMT_SYSTEM_CONFIG=\""$(cf_datadir)/code-format.conf"\"
preview_la_LIBADD = $(GEANY_LIBS) $(WEBKIT_LIBS) $(CMARK_LIBS)
preview_la_LDFLAGS = -module -avoid-version
preview_la_SOURCES = \
	plugin.c plugin.h \
	formats.c formats.h \
	process.c process.h
