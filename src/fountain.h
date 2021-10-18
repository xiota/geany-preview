/* -*- C++ -*-
 * Fountain Screenplay Processor
 * Copyright 2021 xiota
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>
#include <vector>

namespace Fountain {

enum ScriptNodeType : size_t {
  ftnNone = 0,
  ftnUnknown = 1ull,
  ftnKeyValue = 1ull << 1,
  ftnContinuation = 1ull << 2,
  ftnPageBreak = 1ull << 3,
  ftnBlankLine = 1ull << 4,
  ftnSceneHeader = 1ull << 5,
  ftnAction = 1ull << 6,
  ftnTransition = 1ull << 7,
  ftnDialog = 1ull << 8,
  ftnDialogLeft = 1ull << 9,
  ftnDialogRight = 1ull << 10,
  ftnCharacter = 1ull << 11,
  ftnParenthetical = 1ull << 12,
  ftnSpeech = 1ull << 13,
  ftnNotation = 1ull << 14,
  ftnLyric = 1ull << 15,
  ftnSection = 1ull << 16,
  ftnSynopsis = 1ull << 17,
};

class ScriptNode {
 public:
  std::string to_string(size_t const &flags = ScriptNodeType::ftnNone) const;

 public:
  ScriptNodeType type = ScriptNodeType::ftnUnknown;
  std::string key;
  std::string value;
};

class Script {
 public:
  void parseFountain(std::string const &text);
  std::string to_string(size_t const &flags = ScriptNodeType::ftnNone) const;

  std::vector<ScriptNode> nodes;

 private:
  ScriptNode curr_node;

  std::string parseNodeText(std::string const &input);

  void new_node(ScriptNodeType const &type, std::string const &key = "",
                std::string const &value = "") {
    end_node();
    curr_node = {type, key, value};
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
  void append(std::string const &s) {
    if (curr_node.value.size()) {
      curr_node.value += '\n';
    }
    curr_node.value += s;
  }
};

}  // namespace Fountain

// html output compatible with screenplain css files
std::string ftn2screenplain(std::string const &input,
                            std::string const &css_fn = nullptr);

// html output compatible with textplay css files
std::string ftn2textplay(std::string const &input, std::string const &css_fn = nullptr);

// possibly compatible with finaldraft fdx files
std::string ftn2fdx(std::string const &input);

// native output; modern browsers can display with css
std::string ftn2xml(std::string const &input, std::string const &css_fn = nullptr);
