project(
  'geany-preview', 'cpp',
  license: 'GPL-3.0-or-later',
  version: files('version.txt'),
  meson_version: '>=0.61',
  default_options: ['buildtype=release', 'prefix=/usr'],
)

project_name = meson.project_name()
plugin_name  = project_name.startswith('geany-') ? project_name.substring(6) : project_name

geany_dep = dependency('geany')
cmark_dep = dependency('libcmark-gfm')
webkit_dep = dependency('webkit2gtk-4.1')

pdf_export = get_option('pdf_export')
if not pdf_export.disabled()
  podofo_dep = dependency('libpodofo', required: pdf_export)
else
  podofo_dep = disabler()
endif

src_files = files(
  'source/converter_cmark_gfm.cc',
  'source/converter_registrar.cc',
  'source/document.cc',
  'source/preview.cc',
  'source/preview_pane.cc',
)

shared_module(
  plugin_name,
  src_files,
  dependencies: [geany_dep, cmark_dep, podofo_dep, webkit_dep],
  name_prefix: '',
  install: true,
  install_dir: join_paths(geany_dep.get_variable(pkgconfig: 'libdir'), 'geany'),
)
