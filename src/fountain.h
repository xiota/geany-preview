/*
 * C++ Fountain Parser
 * Copyright 2021 xiota
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#pragma once

#include <string>
#include <vector>

enum class ScriptNodeType {
  ftnUnknown,
  ftnKeyValue,
  ftnPageBreak,
  ftnBlankLine,
  ftnContinuation,
  ftnSceneHeader,
  ftnAction,
  ftnTransition,
  ftnDialog,
  ftnDialogLeft,
  ftnDialogRight,
  ftnCharacter,
  ftnParenthetical,
  ftnSpeech,
  ftnNotation,
  ftnLyric,
  ftnSection,
  ftnSynopsis,
};

class ScriptNode {
 public:
  ScriptNodeType type = ScriptNodeType::ftnUnknown;
  std::string key;
  std::string value;
  std::string to_string() const;
};

class Script {
 public:
  Script() {}

  void parseFountain(const std::string &fountainFile);

  std::vector<ScriptNode> nodes;

 private:
  ScriptNode curr_node;

  std::string parseNodeText(const std::string &input);

  void new_node(ScriptNodeType type, const std::string &value) {
    end_node();
    curr_node = {type, value, ""};
  }
  void end_node() {
    if (curr_node.type != ScriptNodeType::ftnUnknown) {
      curr_node.value = parseNodeText(curr_node.value);
      nodes.push_back(curr_node);

      curr_node.type = ScriptNodeType::ftnUnknown;
      curr_node.key = "";
      curr_node.value = "";
    }
  }
  void append_text(const std::string &s) {
    if (curr_node.value.size()) {
      curr_node.value += '\n';
    }
    curr_node.value += s;
  }
};

char *ftn2str(const char *input);

char *ftn2fdx(const char *input);
char *ftn2html(const char *input, const char *css_fn);

char *ftn2html2(const char *input, const char *css_fn);
