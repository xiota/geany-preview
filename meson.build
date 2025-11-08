project(
  'geany-preview', 'cpp',
  license: 'GPL-3.0-or-later',
  version: files('version.txt'),
  meson_version: '>=0.61',
  default_options: ['buildtype=release', 'prefix=/usr', 'cpp_std=c++20'],
)

project_name = meson.project_name()
plugin_name  = project_name.startswith('geany-') ? project_name.substring(6) : project_name
plugin_datadir = get_option('datadir') / 'geany' / 'plugins' / plugin_name

geany_dep = dependency('geany')
tomlpp_dep = dependency('tomlplusplus')
webkit_dep = dependency('webkit2gtk-4.1')

# priority: cmark-gfm, md4c, cmark
markdown_dep = dependency('libcmark-gfm', required: false)
markdown_src = 'source/converter_cmark.cc'
if markdown_dep.found()
  add_project_arguments('-DHAVE_CMARK_GFM', language: 'cpp')
else
  markdown_dep = dependency('md4c-html', required: false)
  if markdown_dep.found()
    warning('libcmark-gfm not found, using md4c')
    add_project_arguments('-DHAVE_MD4C', language: 'cpp')
    markdown_src = 'source/converter_md4c.cc'
  else
    markdown_dep = dependency('libcmark', required: false)
    if markdown_dep.found()
      warning('libcmark-gfm and md4c not found, using plain libcmark')
      add_project_arguments('-DHAVE_CMARK', language: 'cpp')
    else
      error('No markdown library found (libcmark-gfm, md4c, libcmark)')
    endif
  endif
endif

pdf_export = get_option('pdf_export')
if not pdf_export.disabled()
  podofo_dep = dependency('libpodofo', required: pdf_export)
  pdf_export_str = podofo_dep.found() ? 'enabled' : 'disabled'
else
  podofo_dep = disabler()
  pdf_export_str = 'disabled'
endif

ftn2xml_dep = dependency(
  'ftn2xml',
  fallback: ['ftn2xml', 'ftn2xml_dep'],
  default_options: {'pdf_export': pdf_export_str},
)

# Configuration
conf_data = configuration_data()
conf_data.set('version', meson.project_version())
conf_data.set('project_datadir', plugin_datadir)
conf_data.set10('pdf_export', podofo_dep.found())

config_h = configure_file(
  input: 'source/config.h.in',
  output: 'config.h',
  configuration: conf_data
)

# Default CSS
fs = import('fs')

fountain_css_content  = fs.read(meson.project_source_root() / 'data/fountain.css')
preview_css_content  = fs.read(meson.project_source_root() / 'data/preview.css')
markdown_css_content = fs.read(meson.project_source_root() / 'data/markdown.css')

css_conf = configuration_data()
css_conf.set('FOUNTAIN_CSS',  fountain_css_content.strip())
css_conf.set('PREVIEW_CSS',  preview_css_content.strip())
css_conf.set('MARKDOWN_CSS', markdown_css_content.strip())

default_css_h = configure_file(
  input: 'source/default_css.h.in',
  output: 'default_css.h',
  configuration: css_conf
)

src_files = files(
  markdown_src,
  'source/converter_ftn2xml.cc',
  'source/converter_registrar.cc',
  'source/converter_subprocess.cc',
  'source/document.cc',
  'source/preview.cc',
  'source/preview_config.cc',
  'source/preview_menu.cc',
  'source/preview_shortcuts.cc',
  'source/subprocess.cc',
  'source/util/gtk_utils.cc',
  'source/webview.cc',
)

shared_module(
  plugin_name,
  src_files,
  dependencies: [geany_dep, markdown_dep, ftn2xml_dep, podofo_dep, tomlpp_dep, webkit_dep],
  name_prefix: '',
  install: true,
  install_dir: join_paths(geany_dep.get_variable(pkgconfig: 'libdir'), 'geany'),
)
