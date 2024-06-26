project(
  'geany-preview',
  'cpp',
  license : 'GPL-3.0-or-later',
  default_options : ['buildtype=release', 'prefix=/usr']
)

prefix = get_option('prefix')

plugin_name = 'preview'
project_datadir = get_option('datadir') / meson.project_name()
project_docdir = get_option('datadir') / 'doc' / meson.project_name()
plugin_conf = plugin_name + '.conf'

doc_readme = 'Readme.md'
doc_faq = 'docs/CQA.md'

geany = dependency('geany')
cmark = dependency('libcmark-gfm')

webkit2gtk = dependency('webkit2gtk-4.0', required: false)
if not webkit2gtk.found()
  webkit2gtk = dependency('webkit2gtk-4.1')
endif

pdf_export = get_option('pdf_export')
if not pdf_export.disabled()
  podofo = dependency('libpodofo', required: pdf_export)
else
  podofo = disabler()
endif

conf_data = configuration_data()
conf_data.set('version', meson.project_version())

conf_data.set('prefix', prefix)
conf_data.set('project_datadir', project_datadir)

conf_data.set('css_asciidoctor', get_option('css_asciidoctor'))
conf_data.set('css_extra-dark', get_option('css_extra-dark'))
conf_data.set('css_extra-invert', get_option('css_extra-invert'))
conf_data.set('css_extra-media', get_option('css_extra-media'))
conf_data.set('css_fountain', get_option('css_fountain'))
conf_data.set('css_headers', get_option('css_headers'))
conf_data.set('css_markdown', get_option('css_markdown'))
conf_data.set('css_pandoc', get_option('css_pandoc'))

conf_data.set('doc_readme', doc_readme)
conf_data.set('doc_faq', doc_faq)

conf_data.set10('pdf_export', podofo.found())

config_h = configure_file(
  input: 'config.h.in',
  output: 'config.h',
  configuration: conf_data,
)

library(
  plugin_name,
  sources: [
    config_h,
    'source/ftn2xml/auxiliary.cxx',
    'source/ftn2xml/fountain.cxx',
    'source/process.cxx',
    'source/pv_config_dialog.cxx',
    'source/pv_formats.cxx',
    'source/pv_main.cxx',
    'source/pv_settings.cxx',
  ],
  dependencies: [geany, cmark, podofo, webkit2gtk],
  name_prefix: '',
  install: true,
  install_dir: get_option('libdir') / 'geany',
)

if meson.version().version_compare('>=0.58.0')
  # fix readme paths
  fs = import('fs')
  readme_config = configuration_data()
  readme_config.set('output', fs.read(doc_readme).replace('](docs/', '](').strip())
  doc_readme_mod = configure_file(
    input: 'meson.output',
    output: 'Readme.md',
    configuration: readme_config,
  )
else
  doc_readme_mod = doc_readme
endif

install_data(
  sources: [
    'data/asciidoctor.css',
    'data/extra-dark.css',
    'data/extra-invert.css',
    'data/extra-media.css',
    'data/fountain.css',
    'data/headers.css',
    'data/markdown.css',
    'data/pandoc.css',
    'data/std.css',
  ],
  install_dir: project_datadir,
)

install_data(
  sources: [
    doc_faq,
    doc_readme_mod,
    'docs/geany-plugin-preferences.png',
    'docs/screenshot-908.jpg',
  ],
  install_dir: project_docdir,
)
