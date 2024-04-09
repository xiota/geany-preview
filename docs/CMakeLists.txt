
# variables for generated header
set(DOCS_FAQ "CQA.md")
set(DOCS_README "Readme.md")

# fix paths in readme

add_custom_target(docs_readme ALL
  COMMAND sed -E -e "s@\\]\\(docs/([^\\)]+)\\)@](\\1)@" "${CMAKE_CURRENT_SOURCE_DIR}/../${DOCS_README}" > "${CMAKE_CURRENT_BINARY_DIR}/${DOCS_README}"
  VERBATIM USES_TERMINAL
)

# install
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${DOCS_README}"
    "${DOCS_FAQ}"
    geany-plugin-preferences.png
    screenshot-908.jpg
  PERMISSIONS
    OWNER_READ
    OWNER_WRITE
    GROUP_READ
    WORLD_READ
  DESTINATION "${PREVIEW_INSTALL_DOCDIR}"
)

set(PARENT_SCOPE)
