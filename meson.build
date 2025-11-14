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
webkit_dep = dependency('webkit2gtk-4.1')

tomlpp_dep = dependency('tomlplusplus')
tomlpp_headers = declare_dependency(
  include_directories: tomlpp_dep.get_variable(pkgconfig: 'includedir')
)

markdown_backend = get_option('markdown_backend')
markdown_src = 'source/converter_cmark.cc'
if markdown_backend == 'cmark-gfm'
  message('using markdown backend: cmark-gfm')
  markdown_dep = dependency('libcmark-gfm', required: true)
  add_project_arguments('-DHAVE_CMARK_GFM', language: 'cpp')
elif markdown_backend == 'md4c'
  message('using markdown backend: md4c')
  markdown_dep = dependency('md4c-html', required: true)
  add_project_arguments('-DHAVE_MD4C', language: 'cpp')
  markdown_src = 'source/converter_md4c.cc'
elif markdown_backend == 'cmark'
  message('using markdown backend: cmark')
  markdown_dep = dependency('libcmark', required: true)
  add_project_arguments('-DHAVE_CMARK', language: 'cpp')
else
  # auto priority: cmark-gfm, md4c, cmark
  markdown_dep = dependency('libcmark-gfm', required: false)
  if markdown_dep.found()
    add_project_arguments('-DHAVE_CMARK_GFM', language: 'cpp')
    message('using markdown backend: cmark-gfm (auto)')
  else
    markdown_dep = dependency('md4c-html', required: false)
    if markdown_dep.found()
      message('using markdown backend: md4c (auto)')
      add_project_arguments('-DHAVE_MD4C', language: 'cpp')
      markdown_src = 'source/converter_md4c.cc'
    else
      markdown_dep = dependency('libcmark', required: false)
      if markdown_dep.found()
        message('using markdown backend: cmark (auto)')
        add_project_arguments('-DHAVE_CMARK', language: 'cpp')
      else
        error('No markdown library found (cmark-gfm, md4c, cmark)')
      endif
    endif
  endif
endif

podofo_dep = dependency('libpodofo', required: get_option('podofo'))
if podofo_dep.found()
  add_project_arguments('-DHAVE_PODOFO', language: 'cpp')
endif

ftn2xml_dep = dependency(
  'ftn2xml',
  fallback: ['ftn2xml', 'ftn2xml_dep'],
)

# Configuration
conf_data = configuration_data()
conf_data.set('version', meson.project_version())
conf_data.set('project_datadir', plugin_datadir)

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
  'source/document_geany.cc',
  'source/preview.cc',
  'source/preview_config.cc',
  'source/preview_menu.cc',
  'source/preview_pane.cc',
  'source/preview_shortcuts.cc',
  'source/subprocess.cc',
  'source/util/gtk_utils.cc',
  'source/util/xdg_utils.cc',
  'source/webview.cc',
)

shared_module(
  plugin_name,
  src_files,
  dependencies: [geany_dep, markdown_dep, ftn2xml_dep, podofo_dep, tomlpp_headers, webkit_dep],
  name_prefix: '',
  install: true,
  install_dir: join_paths(geany_dep.get_variable(pkgconfig: 'libdir'), 'geany'),
)
