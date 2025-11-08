// SPDX-FileCopyrightText: Copyright 2025 xiota
// SPDX-License-Identifier: GPL-3.0-or-later

#include "converter_md4c.h"

#include <memory>
#include <string>
#include <string_view>

extern "C" {
#include <md4c-html.h>
}

std::string_view ConverterMd4c::toHtml(std::string_view source) {
  unsigned parser_flags = MD_FLAG_TABLES | MD_FLAG_STRIKETHROUGH | MD_FLAG_TASKLISTS |
                          MD_FLAG_PERMISSIVEURLAUTOLINKS | MD_FLAG_PERMISSIVEWWWAUTOLINKS |
                          MD_FLAG_UNDERLINE;

  unsigned renderer_flags = MD_HTML_FLAG_SKIP_UTF8_BOM;

  std::string buffer;

  auto callback = [](const MD_CHAR *text, MD_SIZE size, void *userdata) {
    auto *out = static_cast<std::string *>(userdata);
    out->append(text, size);
  };

  int result = md_html(
      source.data(),
      static_cast<MD_SIZE>(source.size()),
      callback,
      &buffer,
      parser_flags,
      renderer_flags
  );

  if (result != 0) {
    buffer.clear();
  }

  html_owner_ = std::make_unique<std::string>(std::move(buffer));
  html_view_ = std::string_view(html_owner_->data(), html_owner_->size());

  return html_view_;
}
