ACLOCAL_AMFLAGS = -I m4

cf_docdir = $(docdir)
cf_doc_DATA = README.md

cf_datadir = $(datadir)/geany-plugins/@PACKAGE@
cf_data_DATA = \
	data/preview.conf \
	data/markdown.css \
	data/pandoc.css \
	data/screenplain.css

geany_plugindir = $(libdir)/geany
geany_plugin_LTLIBRARIES = preview.la

preview_la_CFLAGS = $(GEANY_CFLAGS) $(WEBKIT_CFLAGS) $(CMARK_CFLAGS) \
	-DG_LOG_DOMAIN=\""Preview"\" \
	-DPREVIEW_README=\""$(cf_docdir)/README.md"\" \
	-DPREVIEW_CONFIG=\""$(pkgdatadir)/preview.conf"\" \
	-DPREVIEW_CSS_PANDOC=\""$(cf_datadir)/pandoc.css"\" \
	-DPREVIEW_CSS_MARKDOWN=\""$(cf_datadir)/markdown.css"\" \
	-DPREVIEW_CSS_SCREENPLAIN=\""$(cf_datadir)/screenplain.css"\"
preview_la_LIBADD = $(GEANY_LIBS) $(WEBKIT_LIBS) $(CMARK_LIBS)
preview_la_LDFLAGS = -module -avoid-version
preview_la_SOURCES = \
	src/plugin.c \
	src/plugin.h \
	src/formats.c \
	src/formats.h \
	src/prefs.c \
	src/prefs.h \
	src/process.c \
	src/process.h
