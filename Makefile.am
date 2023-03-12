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
	data/preview.css \
	data/asciidoctor.css \
	data/fountain.css \
	data/markdown.css \
	data/pandoc.css \
	data/extra-dark.css \
	data/extra-invert.css \
	data/extra-media.css

geany_plugindir = $(libdir)/geany
geany_plugin_LTLIBRARIES = preview.la

preview_la_CXXFLAGS = $(GEANY_CFLAGS) $(WEBKIT_CFLAGS) $(CMARK_CFLAGS) $(PODOFO_CFLAGS) \
	-DG_LOG_DOMAIN=\""Preview"\" \
	-DPREVIEW_README=\""$(cf_docdir)/README.md"\" \
	-DPREVIEW_FAQ=\""$(cf_docdir)/CQA.md"\" \
	-DPREVIEW_CSS_HEADERS=\""$(cf_datadir)/preview.css"\" \
	-DPREVIEW_CSS_PANDOC=\""$(cf_datadir)/pandoc.css"\" \
	-DPREVIEW_CSS_ASCIIDOCTOR=\""$(cf_datadir)/asciidoctor.css"\" \
	-DPREVIEW_CSS_FOUNTAIN=\""$(cf_datadir)/fountain.css"\" \
	-DPREVIEW_CSS_MARKDOWN=\""$(cf_datadir)/markdown.css"\" \
	-DPREVIEW_CSS_EXTRA_DARK=\""$(cf_datadir)/extra-dark.css"\" \
	-DPREVIEW_CSS_EXTRA_INVERT=\""$(cf_datadir)/extra-invert.css"\" \
	-DPREVIEW_CSS_EXTRA_MEDIA=\""$(cf_datadir)/extra-media.css"\"

preview_la_LIBADD = $(GEANY_LIBS) $(WEBKIT_LIBS) $(CMARK_LIBS) $(PODOFO_LIBS)
preview_la_LDFLAGS = -module -avoid-version

preview_la_SOURCES = \
	src/pv_main.cc \
	src/pv_main.h \
	src/pv_settings.cc \
	src/pv_settings.h \
	src/pv_formats.cc \
	src/pv_formats.h \
	src/process.cc \
	src/process.h \
	src/auxiliary.cc \
	src/auxiliary.h \
	src/fountain.cc \
	src/fountain.h
