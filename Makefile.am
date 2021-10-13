ACLOCAL_AMFLAGS = -I m4

cf_docdir = $(docdir)
cf_doc_DATA = \
	docs/Building_on_Linux.md \
	docs/geany-plugin-preferences.png \
	docs/CQA.md \
	docs/screenshot-908.jpg \
	Readme.md

cf_datadir = $(datadir)/geany-plugins/@PACKAGE@
cf_data_DATA = \
	data/preview.conf \
	data/preview-headers.css \
	data/asciidoctor.css \
	data/markdown.css \
	data/pandoc.css \
	data/screenplain.css \
	data/dark.css \
	data/invert.css

geany_plugindir = $(libdir)/geany
geany_plugin_LTLIBRARIES = preview.la

preview_la_CXXFLAGS = $(GEANY_CFLAGS) $(WEBKIT_CFLAGS) $(CMARK_CFLAGS) \
	-DG_LOG_DOMAIN=\""Preview"\" \
	-DPREVIEW_README=\""$(cf_docdir)/README.md"\" \
	-DPREVIEW_FAQ=\""$(cf_docdir)/CQA.md"\" \
	-DPREVIEW_CONFIG=\""$(cf_datadir)/preview.conf"\" \
	-DPREVIEW_CSS_HEADERS=\""$(cf_datadir)/preview-headers.css"\" \
	-DPREVIEW_CSS_PANDOC=\""$(cf_datadir)/pandoc.css"\" \
	-DPREVIEW_CSS_ASCIIDOCTOR=\""$(cf_datadir)/asciidoctor.css"\" \
	-DPREVIEW_CSS_MARKDOWN=\""$(cf_datadir)/markdown.css"\" \
	-DPREVIEW_CSS_SCREENPLAIN=\""$(cf_datadir)/screenplain.css"\" \
	-DPREVIEW_CSS_DARK=\""$(cf_datadir)/dark.css"\" \
	-DPREVIEW_CSS_INVERT=\""$(cf_datadir)/invert.css"\"
preview_la_LIBADD = $(GEANY_LIBS) $(WEBKIT_LIBS) $(CMARK_LIBS)
preview_la_LDFLAGS = -module -avoid-version
preview_la_SOURCES = \
	src/plugin.cc \
	src/plugin.h \
	src/formats.cc \
	src/formats.h \
	src/prefs.cc \
	src/prefs.h \
	src/process.cc \
	src/process.h \
	src/fountain.cc \
	src/fountain.h
