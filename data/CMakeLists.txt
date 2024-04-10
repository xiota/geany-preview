
# variables for header
if(NOT DEFINED CSS_ASCIIDOCTOR)
  set(CSS_ASCIIDOCTOR "asciidoctor.css")
endif()
if(NOT DEFINED CSS_EXTRA_DARK)
  set(CSS_EXTRA_DARK "extra-dark.css")
endif()
if(NOT DEFINED CSS_EXTRA_INVERT)
  set(CSS_EXTRA_INVERT "extra-invert.css")
endif()
if(NOT DEFINED CSS_EXTRA_MEDIA)
  set(CSS_EXTRA_MEDIA "extra-media.css")
endif()
if(NOT DEFINED CSS_FOUNTAIN)
  set(CSS_FOUNTAIN "fountain.css")
endif()
if(NOT DEFINED CSS_HEADERS)
  set(CSS_HEADERS "preview.css")
endif()
if(NOT DEFINED CSS_MARKDOWN)
  set(CSS_MARKDOWN "markdown.css")
endif()
if(NOT DEFINED CSS_PANDOC)
  set(CSS_PANDOC "pandoc.css")
endif()

# install
install(
  FILES
    ${CSS_ASCIIDOCTOR}
    ${CSS_EXTRA_DARK}
    ${CSS_EXTRA_INVERT}
    ${CSS_EXTRA_MEDIA}
    ${CSS_FOUNTAIN}
    ${CSS_HEADERS}
    ${CSS_MARKDOWN}
    ${CSS_PANDOC}
  PERMISSIONS
    OWNER_READ
    OWNER_WRITE
    GROUP_READ
    WORLD_READ
  DESTINATION "${GEANY_INSTALL_PLUGIN_DATA}/${PROJECT_NAME}"
)

set(PARENT_SCOPE)
