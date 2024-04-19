/*
 * SPDX-FileCopyrightText: Copyright 2021-2024 xiota
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <map>
#include <string>
#include <vector>

#include "config.h"

namespace Fountain {

enum ScriptNodeType : unsigned long long {
  ftnNone = 0,
  ftnUnknown = 1ull,
  ftnBoneyard = 1ull << 1,
  ftnComment = 1ull << 2,
  ftnKeyValue = 1ull << 3,
  ftnContinuation = 1ull << 4,
  ftnPageBreak = 1ull << 5,
  ftnBlankLine = 1ull << 6,
  ftnSceneHeader = 1ull << 7,
  ftnAction = 1ull << 8,
  ftnActionCenter = 1ull << 9,
  ftnTransition = 1ull << 10,
  ftnDialog = 1ull << 11,
  ftnDialogLeft = 1ull << 12,
  ftnDialogRight = 1ull << 13,
  ftnCharacter = 1ull << 14,
  ftnParenthetical = 1ull << 15,
  ftnSpeech = 1ull << 16,
  ftnNotation = 1ull << 17,
  ftnLyric = 1ull << 18,
  ftnSection = 1ull << 19,
  ftnSynopsis = 1ull << 20,
};

class ScriptNode {
 public:
  std::string to_string(const int & flags = ScriptNodeType::ftnNone) const;

  void clear() {
    type = ScriptNodeType::ftnUnknown;
    key.clear();
    value.clear();
  }

 public:
  ScriptNodeType type = ScriptNodeType::ftnUnknown;
  std::string key;
  std::string value;
};

class Script {
 public:
  Script() = default;
  Script(const std::string & text) {
    parseFountain(text);
  }

  void clear() {
    nodes.clear();
    curr_node.clear();
  }

  void parseFountain(const std::string & text);
  std::string to_string(const int & flags = ScriptNodeType::ftnNone) const;

 public:
  std::vector<ScriptNode> nodes;
  std::map<std::string, std::string> metadata;

 private:
  ScriptNode curr_node;

  std::string parseNodeText(const std::string & input);

  void new_node(
      const ScriptNodeType & type, const std::string & key = "", const std::string & value = ""
  ) {
    end_node();
    curr_node = {type, key, value};
  }
  void end_node() {
    if (curr_node.type != ScriptNodeType::ftnUnknown) {
      curr_node.value = parseNodeText(curr_node.value);
      nodes.push_back(curr_node);
      curr_node.clear();
    }
  }
  void append(const std::string & s) {
    if (curr_node.value.size()) {
      curr_node.value += '\n';
    }
    curr_node.value += s;
  }
};

// html output compatible with screenplain css files
std::string ftn2screenplain(
    const std::string & input, const std::string & css_fn = "screenplain.css",
    const bool & embed_css = false
);

// html output compatible with textplay css files
std::string ftn2textplay(
    const std::string & input, const std::string & css_fn = "textplay.css",
    const bool & embed_css = false
);

// possibly compatible with finaldraft fdx files
std::string ftn2fdx(const std::string & input);

// native output; modern browsers can display with css
std::string ftn2xml(
    const std::string & input, const std::string & css_fn = "fountain-xml.css",
    const bool & embed_css = false
);
std::string ftn2html(
    const std::string & input, const std::string & css_fn = "fountain-html.css",
    const bool & embed_css = false
);

#if ENABLE_EXPORT_PDF
bool ftn2pdf(const std::string & fn, const std::string & input);
#endif  // ENABLE_EXPORT_PDF

}  // namespace Fountain
