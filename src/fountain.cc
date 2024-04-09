/*
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

#include "fountain.h"

#if ENABLE_EXPORT_PDF
#include <podofo/podofo.h>
#endif  // ENABLE_EXPORT_PDF

#include <cstring>

#include "auxiliary.h"

namespace Fountain {

namespace {

bool isForced(std::string const &input) {
  if (input.empty()) {
    return false;
  }

  switch (input[0]) {
    case '#':  // section
    case '=':  // synposis / pagebrek
    case '>':  // transition / center
    case '!':  // action
    case '.':  // scene header
    case '(':  // parenthetical
    case '@':  // character
    case '*':  // bold / italic
    case '_':  // underline
    case '~':  // lyric
      return true;
  }

  return false;
}

bool isTransition(std::string const &input) {
  if (input.empty()) {
    return false;
  }

  // Note: input should already be left-trimmed,
  // otherwise indent would be removed here

  const std::size_t len = input.length();

  // forced transition; make sure intent is not to center
  if (input[0] == '>' && input[input.length() - 1] != '<') {
    return true;
  }

  if (isForced(input)) {
    return false;
  }

  // must be a reasonable length
  if (len > 20 || len < 5) {
    return false;
  }

  // must be uppercase
  if (!is_upper(input)) {
    return false;
  }

  // "CUT TO:", "DISSOLVE TO:", "FADE TO:", "FLASH CUT TO:", "JUMP CUT TO:",
  // "MATCH CUT TO:", "MATCH DISSOLVE TO:", "SMASH CUT TO:", "WIPE TO:"
  std::string compare = input.substr(len - 4);
  if (compare == std::string(" TO:")) {
    return true;
  }

  // "FADE OUT.", "IRIS OUT."
  compare = input.substr(len - 5);
  if (compare == std::string(" OUT.")) {
    return true;
  }

  // "CUT TO BLACK."
  compare = input.substr(0, 7);
  if (compare == std::string("CUT TO ")) {
    return true;
  }

  // Other transitions
  const char *transitions[] = {
      "DISSOLVE:",      "END CREDITS:", "FADE IN:",         "FREEZE FRAME:",
      "INTERCUT WITH:", "IRIS IN:",     "OPENING CREDITS:", "SPLIT SCREEN:",
      "STOCK SHOT:",    "TIME CUT:",    "TITLE OVER:",      "WIPE:",
  };

  for (auto str : transitions) {
    if (input == std::string(str)) {
      return true;
    }
  }

  return false;
}

std::string parseTransition(std::string const &input) {
  if (input.empty()) {
    return {};
  }
  if (input[0] == '>') {
    return to_upper(ws_trim(input.substr(1)));
  }
  return to_upper(ws_trim(input));
}

bool isSceneHeader(std::string const &input) {
  if (input.length() < 2) {
    return false;
  }

  if (input[0] == '.' && input[1] != '.') {
    return true;
  }

  if (isForced(input)) {
    return false;
  }

  try {
    static const std::regex re_scene_header(
        R"(^(INT|EXT|EST|INT\.?/EXT|EXT\.?/INT|I/E|E/I)[\.\ ])",
        std::regex_constants::icase);

    if (std::regex_search(input, re_scene_header)) {
      return true;
    }
  } catch (std::regex_error &e) {
    print_regex_error(e, __FILE__, __LINE__);
  }
  return false;
}

// returns <scene description, scene number>
// Note: in should already be length checked by isSceneHeader()
auto parseSceneHeader(std::string const &input) {
  std::string first;
  std::string second;
  bool forced_scene{false};

  if (input[0] == '.' && input[1] != '.') {
    forced_scene = true;
  }

  const std::size_t pos = input.find("#");
  if (pos < input.length() - 1 && input.back() == '#') {
    if (forced_scene) {
      first = ws_trim(input.substr(1, pos - 1));
      second = ws_trim(input.substr(pos + 1, input.length() - pos - 3));
    } else {
      first = ws_trim(input.substr(0, pos));
      second = ws_trim(input.substr(pos + 1, input.length() - pos - 2));
    }
  } else {
    first = ws_trim(input.substr(forced_scene ? 1 : 0));
  }

  return std::make_pair(first, second);
}

bool isCenter(std::string const &input) {
  if (input.length() < 2) {
    return false;
  }

  // Note: input should already be left-trimmed,
  // otherwise indent would be removed here

  // TODO: Should input also be right-trimmed?
  if (input[0] == '>' && input[input.length() - 1] == '<') {
    return true;
  }

  return false;
}

bool isNotation(std::string const &input) {
  if (input.length() < 4) {
    return false;
  }

  // input should already be left-trimmed...
  if (input[0] != '[' || input[1] != '[') {
    return false;
  }

  // check if right-trimmed to avoid copying string
  static const std::string strWhiteSpace{FOUNTAIN_WHITESPACE};
  if (strWhiteSpace.find(input.back()) != std::string::npos) {
    std::string s = ws_rtrim(input);
    const std::size_t len = s.length();
    if (s[len - 1] == ']' && s[len - 2] == ']') {
      return true;
    }
  } else {
    const std::size_t len = input.length();
    if (input[len - 1] == ']' && input[len - 2] == ']') {
      return true;
    }
  }

  return false;
}

bool isCharacter(std::string const &input) {
  if (input.empty()) {
    return false;
  }

  // Note: input should already be left-trimmed,
  // otherwise indent would be removed here

  // forced character
  if (input[0] == '@') {
    return true;
  }

  if (isForced(input)) {
    return false;
  }

  // check for same-line parenthetical
  std::size_t pos = input.find("(");
  if (pos != std::string::npos && input.find(")") != std::string::npos) {
    if (is_upper(input.substr(0, pos))) {
      return true;
    }
  } else if (is_upper(input)) {
    return true;
  }

  return false;
}

// Note: input should already be length checked by isCharacter()
std::string parseCharacter(std::string const &input) {
  // Note: input should already be left-trimmed,
  // otherwise indent would be removed here

  std::string output;

  if (input[0] == '@') {
    output = ws_ltrim(input.substr(1));
  } else {
    output = input;
  }

  if (output.back() == '^') {
    output = ws_rtrim(output.substr(0, output.length() - 1));
  }

  return output;
}

// Note: input should already be length checked by isCharacter()
bool isDualDialog(std::string const &input) {
  if (input.back() == '^') {
    return true;
  }

  return false;
}

bool isParenthetical(std::string const &input) {
  if (!input.length()) {
    return false;
  }

  // input should already be left=trimmed.
  if (input[0] != '(') {
    return false;
  }

  // check if right-trimmed to avoid copying string
  static std::string strWhiteSpace{FOUNTAIN_WHITESPACE};
  if (strWhiteSpace.find(input.back()) != std::string::npos) {
    std::string s = ws_rtrim(input);
    const std::size_t len = s.length();
    if (s[len - 1] == ')') {
      return true;
    }
  } else {
    const std::size_t len = input.length();
    if (input[len - 1] == ')') {
      return true;
    }
  }
  return false;
}

bool isContinuation(std::string const &input) {
  if (input.empty()) {
    return false;
  }

  if (input.find_first_not_of(FOUNTAIN_WHITESPACE) == std::string::npos) {
    return true;
  }
  return false;
}

auto parseKeyValue(std::string const &input) {
  std::string key;
  std::string value;

  std::size_t pos = input.find(':');

  if (pos != std::string::npos) {
    key = to_lower(ws_trim(input.substr(0, pos)));
    value = ws_trim(input.substr(pos + 1));
  }

  return std::make_pair(key, value);
}

// replace escape sequences with entities in a single pass
std::string &parseEscapeSequences_inplace(std::string &input) {
  for (std::size_t pos = 0; pos < input.length();) {
    switch (input[pos]) {
      case '\t':
        input.replace(pos, 1, "    ");
        pos += 4;
        break;
      case '\\':
        if (pos + 1 < input.length()) {
          switch (input[pos + 1]) {
            case '&':
              input.replace(pos, 2, "&#38;");
              pos += 5;
              break;
            case '*':
              input.replace(pos, 2, "&#42;");
              pos += 5;
              break;
            case '_':
              input.replace(pos, 2, "&#95;");
              pos += 5;
              break;
            case ':':
              input.replace(pos, 2, "&#58;");
              pos += 5;
              break;
            case '[':
              input.replace(pos, 2, "&#91;");
              pos += 5;
              break;
            case ']':
              input.replace(pos, 2, "&#93;");
              pos += 5;
              break;
            case '\\':
              input.replace(pos, 2, "&#92;");
              pos += 5;
              break;
            case '<':
              input.replace(pos, 2, "&#60;");
              pos += 5;
              break;
            case '>':
              input.replace(pos, 2, "&#62;");
              pos += 5;
              break;
            case '.':
              input.replace(pos, 2, "&#46;");
              pos += 5;
              break;
            default:
              pos += 2;
              break;
          }
        } else {
          ++pos;
        }
        break;
      default:
        ++pos;
        break;
    }
  }
  return input;
}

}  // namespace

std::string ScriptNode::to_string(int const &flags) const {
  static int dialog_state = 0;
  std::string output;

  switch (type) {
    case ScriptNodeType::ftnKeyValue:
      if (flags & type) {
        break;
      }
      output = "<meta>\n<key>" + key + "</key>\n<value>" + value +
               "</value>\n</meta>\n";
      break;
    case ScriptNodeType::ftnPageBreak:
      if (flags & type) {
        break;
      }
      if (dialog_state) {
        dialog_state == 1   ? output = "</Dialog>\n"
        : dialog_state == 2 ? output = "</DialogLeft>\n"
                            : output = "</DialogRight>\n</DualDialog>\n";
        dialog_state = 0;
      }
      output += "<PageBreak></PageBreak>\n";
      break;
    case ScriptNodeType::ftnBlankLine:
      if (flags & type) {
        break;
      }
      if (dialog_state) {
        dialog_state == 1   ? output = "</Dialog>"
        : dialog_state == 2 ? output = "</DialogLeft>"
                            : output = "</DialogRight></DualDialog>";
        dialog_state = 0;
      }
      output += "<BlankLine></BlankLine>\n";
      break;
    case ScriptNodeType::ftnContinuation:
      if (flags & type) {
        break;
      }
      output = "<Continuation>" + value + "</Continuation>\n";
      break;
    case ScriptNodeType::ftnSceneHeader:
      if (flags & type) {
        break;
      }
      if (!key.empty()) {
        output = "<SceneHeader><SceneNumL>" + key + "</SceneNumL>" + value +
                 +"<SceneNumR>" + key + "</SceneNumR></SceneHeader>\n";
      } else {
        output = "<SceneHeader>" + value + "</SceneHeader>\n";
      }
      break;
    case ScriptNodeType::ftnAction:
      if (flags & type) {
        break;
      }
      output = "<Action>" + value + "</Action>\n";
      break;
    case ScriptNodeType::ftnActionCenter:
      if (flags & type) {
        break;
      }
      output = "<ActionCenter>" + value + "</ActionCenter>\n";
      break;
    case ScriptNodeType::ftnTransition:
      if (flags & type) {
        break;
      }
      output = "<Transition>" + value + "</Transition>\n";
      break;
    case ScriptNodeType::ftnDialog:
      if (flags & type) {
        break;
      }
      dialog_state = 1;
      output = "<Dialog>" + key;
      break;
    case ScriptNodeType::ftnDialogLeft:
      if (flags & type) {
        break;
      }
      dialog_state = 2;
      output = "<DualDialog><DialogLeft>" + value;
      break;
    case ScriptNodeType::ftnDialogRight:
      if (flags & type) {
        break;
      }
      dialog_state = 3;
      output = "<DialogRight>" + value;
      break;
    case ScriptNodeType::ftnCharacter:
      if (flags & type) {
        break;
      }
      output = "<Character>" + value + "</Character>\n";
      break;
    case ScriptNodeType::ftnParenthetical:
      if (flags & type) {
        break;
      }
      output = "<Parenthetical>" + value + "</Parenthetical>\n";
      break;
    case ScriptNodeType::ftnSpeech:
      if (flags & type) {
        break;
      }
      output = "<Speech>" + value + "</Speech>\n";
      break;
    case ScriptNodeType::ftnLyric:
      if (flags & type) {
        break;
      }
      output = "<Lyric>" + value + "</Lyric>\n";
      break;
    case ScriptNodeType::ftnNotation:
      if (flags & type) {
        break;
      }
      output = "<Note>" + value + "</Note>\n";
      break;
    case ScriptNodeType::ftnSection:
      if (flags & type) {
        break;
      }
      output = "<SectionH" + key + ">" + value + "</SectionH" + key + ">\n";
      break;
    case ScriptNodeType::ftnSynopsis:
      if (flags & type) {
        break;
      }
      output = "<SynopsisH" + key + ">" + value + "</SynopsisH" + key + ">\n";
      break;
    case ScriptNodeType::ftnUnknown:
    default:
      if (flags & type) {
        break;
      }
      output = "<Unknown>" + value + "</Unknown>\n";
      break;
  }
  return output;
}

std::string Script::to_string(int const &flags) const {
  std::string output{"<Fountain>\n"};

  for (auto node : nodes) {
    output += node.to_string(flags);
  }

  output += "\n</Fountain>\n";
  return output;
}

std::string Script::parseNodeText(std::string const &input) {
  try {
    static const std::regex re_bolditalic(R"(\*{3}([^*]+?)\*{3})");
    static const std::regex re_bold(R"(\*{2}([^*]+?)\*{2})");
    static const std::regex re_italic(R"(\*{1}([^*]+?)\*{1})");
    static const std::regex re_underline(R"(_([^_\n]+)_)");
    std::string output =
        std::regex_replace(input, re_bolditalic, "<b><i>$1</i></b>");
    output = std::regex_replace(output, re_bold, "<b>$1</b>");
    output = std::regex_replace(output, re_italic, "<i>$1</i>");
    output = std::regex_replace(output, re_underline, "<u>$1</u>");

    static const std::regex re_note_1(R"(\[{2}([\S\s]*?)\]{2})");
    static const std::regex re_note_2(R"(\[{2}([\S\s]*?)$)");
    static const std::regex re_note_3(R"(^([\S\s]*?)\]{2})");
    output = std::regex_replace(output, re_note_1, "<note>$1</note>");
    output = std::regex_replace(output, re_note_2, "<note>$1</note>");
    output = std::regex_replace(output, re_note_3, "<note>$1</note>");

    return output;
  } catch (std::regex_error &e) {
    print_regex_error(e, __FILE__, __LINE__);
    return input;
  }
}

void Script::parseFountain(std::string const &text) {
  clear();

  if (!text.length()) {
    return;
  }

  std::vector<std::string> lines;
  try {
    // remove comments
    static const std::regex re_comment(R"(/\*[\S\s]*?\*/)");
    std::string strTmp = std::regex_replace(text, re_comment, "");

    // replace non-entity, non-escaped ampersands
    static const std::regex re_ampersand(R"(([^\\])&(?!#?[a-zA-Z0-9]+;))");
    strTmp = std::regex_replace(strTmp, re_ampersand, "$1&#38;");

    parseEscapeSequences_inplace(strTmp);

    lines = split_lines(strTmp);
  } catch (std::regex_error &e) {
    print_regex_error(e, __FILE__, __LINE__);
  }

  // determine whether to try to extract header
  bool has_header = false;
  try {
    // check if first line matches header format
    static const std::regex re_has_header(R"(^[^\s:]+:\s)");
    if (std::regex_search(text, re_has_header)) {
      has_header = true;
    }
  } catch (std::regex_error &e) {
    print_regex_error(e, __FILE__, __LINE__);
  }

  // used for synposis
  int currSection = 1;

  // process line-by-line
  for (auto &line : lines) {
    std::string s = ws_ltrim(line);

    if (has_header) {
      if (s.find(':') != std::string::npos) {
        metadata[curr_node.key] = trim_inplace(curr_node.value);
        auto kv = parseKeyValue(s);
        if (!kv.first.empty()) {
          new_node(ScriptNodeType::ftnKeyValue, kv.first);
        }
        if (!kv.second.empty()) {
          append(kv.second);
        }
        continue;
      }
      if (line.length() == 0) {
        metadata[curr_node.key] = trim_inplace(curr_node.value);
        end_node();
        has_header = false;
        continue;
      } else {
        trim_inplace(s);
        if (!s.empty()) {
          append(s);
        }
        continue;
      }
    }

    // Blank Line
    if (line.length() == 0) {
      new_node(ScriptNodeType::ftnBlankLine, line);
      end_node();
      continue;
    }

    // Continuation, line with only whitespace
    if (isContinuation(line)) {
      if (curr_node.type != ScriptNodeType::ftnUnknown) {
        append(" ");
      } else {
        new_node(ScriptNodeType::ftnContinuation, line);
        end_node();
      }
      continue;
    }

    if (isNotation(s)) {
      if (line[0] == ' ' || line[0] == '\t' || line[line.length() - 1] == ' ' ||
          line[line.length() - 1] == '\t') {
        // treat as continuation
        append(s);
      } else {
        // add notation to previous node
        append(s);
        // then insert blank line
        new_node(ScriptNodeType::ftnBlankLine);
        end_node();
      }
      continue;
    }

    // Page Break
    if (begins_with(s, "===")) {
      new_node(ScriptNodeType::ftnPageBreak, s);
      end_node();
      continue;
    }

    // Transition
    if (curr_node.type == ScriptNodeType::ftnUnknown && isTransition(s)) {
      new_node(ScriptNodeType::ftnTransition);
      append(parseTransition(s));
      end_node();
      continue;
    }

    // Scene Header
    if (curr_node.type == ScriptNodeType::ftnUnknown && isSceneHeader(s)) {
      auto scene = parseSceneHeader(s);

      new_node(ScriptNodeType::ftnSceneHeader, scene.second);
      append(scene.first);
      end_node();
      continue;
    }

    // Parenthetical
    if (isParenthetical(s)) {
      if (!nodes.empty()) {
        ScriptNodeType ct = nodes.back().type;
        if (ct == ScriptNodeType::ftnParenthetical ||
            ct == ScriptNodeType::ftnCharacter ||
            ct == ScriptNodeType::ftnSpeech) {
          new_node(ScriptNodeType::ftnParenthetical);
          append(ws_trim(s));
          end_node();
          continue;
        }
      }
    }

    // Speech / Lyric
    if (curr_node.type == ScriptNodeType::ftnSpeech) {
      if (s.length() > 1 && s[0] == '~') {
        new_node(ScriptNodeType::ftnLyric);
        append(s.substr(1));
        continue;
      } else {
        append(s);
        continue;
      }
    } else if (curr_node.type == ScriptNodeType::ftnLyric) {
      if (s.length() > 1 && s[0] == '~') {
        append(s.substr(1));
        continue;
      } else {
        new_node(ScriptNodeType::ftnSpeech);
        append(s);
        continue;
      }
    } else if (!nodes.empty()) {
      ScriptNodeType ct = nodes.back().type;
      if (ct == ScriptNodeType::ftnParenthetical ||
          ct == ScriptNodeType::ftnCharacter) {
        if (s.length() > 1 && s[0] == '~') {
          new_node(ScriptNodeType::ftnLyric);
          append(ws_ltrim(s.substr(1)));
          continue;
        } else {
          new_node(ScriptNodeType::ftnSpeech);
          append(s);
          continue;
        }
      }
    }

    // Character
    if (curr_node.type == ScriptNodeType::ftnUnknown && isCharacter(s)) {
      if (isDualDialog(s)) {
        // modify previous dialog node
        bool prev_dialog_modded = false;
        for (std::size_t pos = nodes.size() - 1; pos >= 0; pos--) {
          if (nodes[pos].type == ScriptNodeType::ftnDialog) {
            nodes[pos].type = ScriptNodeType::ftnDialogLeft;
            prev_dialog_modded = true;
            break;
          }
        }
        if (prev_dialog_modded) {
          new_node(ScriptNodeType::ftnDialogRight);
        } else {
          new_node(ScriptNodeType::ftnDialog);
        }

        new_node(ScriptNodeType::ftnCharacter);
        append(parseCharacter(s));
        end_node();
      } else {
        new_node(ScriptNodeType::ftnDialog);
        new_node(ScriptNodeType::ftnCharacter);
        append(parseCharacter(s));
        end_node();
      }
      continue;
    }

    // Isolated Lyric
    if (s.length() > 1 && s[0] == '~') {
      new_node(ScriptNodeType::ftnLyric);
      append(ws_ltrim(s.substr(1)));
      end_node();
      continue;
    }

    // Section, can only be forced
    if (s.length() > 0 && s[0] == '#') {
      for (std::size_t i = 1; i < 6; ++i) {
        if (s.length() > i && s[i] == '#') {
          if (i == 5) {
            new_node(ScriptNodeType::ftnSection, std::to_string(i + 1));
            append(s.substr(i + 1));
            currSection = i + 1;
            break;
          }
        } else {
          new_node(ScriptNodeType::ftnSection, std::to_string(i));
          append(s.substr(i));
          currSection = i;
          break;
        }
      }
      end_node();
      continue;
    }

    // Synopsis, can only be forced
    if (s.length() > 1 && s[0] == '=') {
      new_node(ScriptNodeType::ftnSynopsis, std::to_string(currSection));
      append(ws_trim(s.substr(1)));
      end_node();
      continue;
    }

    // Probably Action
    // Note: Leading whitespace is retained in Action
    if (curr_node.type == ScriptNodeType::ftnAction) {
      if (isCenter(s)) {
        new_node(ScriptNodeType::ftnActionCenter);
        append(ws_trim(s.substr(1, s.length() - 2)));
        end_node();
      } else {
        append(line);
      }
      continue;
    }
    if (isCenter(s)) {
      new_node(ScriptNodeType::ftnActionCenter);
      append(ws_trim(s.substr(1, s.length() - 2)));
      end_node();
      continue;
    }
    if (s.length() > 1 && s[0] == '!') {
      new_node(ScriptNodeType::ftnAction);
      append(s.substr(1));
      continue;
    }
    if (curr_node.type == ScriptNodeType::ftnUnknown) {
      new_node(ScriptNodeType::ftnAction);
      append(line);
      continue;
    }
  }

  end_node();
}

// html similar to screenplain html output (can use the same css files)
std::string ftn2screenplain(std::string const &input, std::string const &css_fn,
                            bool const &embed_css) {
  std::string output{"<!DOCTYPE html>\n<html>\n<head>\n"};

  if (!css_fn.empty()) {
    if (embed_css) {
      std::string css_contents = file_get_contents(css_fn);

      output += "<style type='text/css'>\n";
      output += css_contents;
      output += "\n</style>\n";
    } else {
      output += R"(<link rel="stylesheet" type="text/css" href=")";
      output += ((css_fn[0] == '/') ? "file://" : "") + css_fn;
      output += "'>\n";
    }
  }

  output +=
      "</head>\n<body>\n"
      "<div id=\"wrapper\" class=\"fountain\">\n";

  Fountain::Script script;
  script.parseFountain(input);

  output += script.to_string(Fountain::ScriptNodeType::ftnContinuation |
                             Fountain::ScriptNodeType::ftnKeyValue |
                             Fountain::ScriptNodeType::ftnUnknown);

  output += "\n</div>\n</body>\n</html>\n";

  try {
    replace_all_inplace(output, "<Transition>", R"(<div class="transition">)");
    replace_all_inplace(output, "</Transition>", "</div>");

    replace_all_inplace(output, "<SceneHeader>", R"(<h6 class="sceneheader">)");
    replace_all_inplace(output, "</SceneHeader>", "</h6>");

    replace_all_inplace(output, "<Action>", R"(<div class="action">)");
    replace_all_inplace(output, "</Action>", "</div>");

    replace_all_inplace(output, "<Lyric>", R"(<div class="lyric">)");
    replace_all_inplace(output, "</Lyric>", "</div>");

    replace_all_inplace(output, "<Character>", R"(<p class="character">)");
    replace_all_inplace(output, "</Character>", "</p>");

    replace_all_inplace(output, "<Parenthetical>",
                        R"(<p class="parenthetical">)");
    replace_all_inplace(output, "</Parenthetical>", "</p>");

    replace_all_inplace(output, "<Speech>", R"(<p class="speech">)");
    replace_all_inplace(output, "</Speech>", "</p>");

    replace_all_inplace(output, "<Dialog>", R"(<div class="dialog">)");
    replace_all_inplace(output, "</Dialog>", "</div>");

    replace_all_inplace(output, "<DialogDual>", R"(<div class="dual">)");
    replace_all_inplace(output, "</DialogDual>", "</div>");

    replace_all_inplace(output, "<DialogLeft>", R"(<div class="left">)");
    replace_all_inplace(output, "</DialogLeft>", "</div>");

    replace_all_inplace(output, "<DialogRight>", R"(<div class="right">)");
    replace_all_inplace(output, "</DialogRight>", "</div>");

    replace_all_inplace(output, "<PageBreak>", R"(<div class="page-break">)");
    replace_all_inplace(output, "</PageBreak>", "</div>");

    replace_all_inplace(output, "<Note>", R"(<div class="note">)");
    replace_all_inplace(output, "</Note>", "</div>");

    replace_all_inplace(output, "<ActionCenter>", R"(<center>)");
    replace_all_inplace(output, "</ActionCenter>", "</center>");

    replace_all_inplace(output, "<BlankLine>", "");
    replace_all_inplace(output, "</BlankLine>", "");

    static const std::regex re_newlines(R"(\n+)");
    output = std::regex_replace(output, re_newlines, "\n");
  } catch (std::regex_error &e) {
    print_regex_error(e, __FILE__, __LINE__);
  }

  return output;
}

// html similar to textplay html output (can use the same css files)
std::string ftn2textplay(std::string const &input, std::string const &css_fn,
                         bool const &embed_css) {
  std::string output{"<!DOCTYPE html>\n<html>\n<head>\n"};

  if (!css_fn.empty()) {
    if (embed_css) {
      std::string css_contents = file_get_contents(css_fn);

      output += "<style type='text/css'>\n";
      output += css_contents;
      output += "\n</style>\n";
    } else {
      output += R"(<link rel="stylesheet" type="text/css" href=")";
      output += ((css_fn[0] == '/') ? "file://" : "") + css_fn;
      output += "'>\n";
    }
  }

  output +=
      "</head>\n<body>\n"
      "<div id=\"wrapper\" class=\"fountain\">\n";

  Fountain::Script script;
  script.parseFountain(input);

  output += script.to_string(Fountain::ScriptNodeType::ftnContinuation |
                             Fountain::ScriptNodeType::ftnKeyValue |
                             Fountain::ScriptNodeType::ftnUnknown);

  output += "\n</div>\n</body>\n</html>\n";

  try {
    replace_all_inplace(output, "<Transition>",
                        R"(<h3 class="right-transition">)");
    replace_all_inplace(output, "</Transition>", "</h3>");

    replace_all_inplace(output, "<SceneHeader>",
                        R"(<h2 class="full-slugline">)");
    replace_all_inplace(output, "</SceneHeader>", "</h2>");

    replace_all_inplace(output, "<Action>", R"(<p class="action">)");
    replace_all_inplace(output, "</Action>", "</p>");

    replace_all_inplace(output, "<Lyric>", R"(<span class="lyric">)");
    replace_all_inplace(output, "</Lyric>", "</span>");

    replace_all_inplace(output, "<Character>", R"(<dt class="character">)");
    replace_all_inplace(output, "</Character>", "</dt>");

    replace_all_inplace(output, "<Parenthetical>",
                        R"(<dd class="parenthetical">)");
    replace_all_inplace(output, "</Parenthetical>", "</dd>");

    replace_all_inplace(output, "<Speech>", R"(<dd class="dialogue">)");
    replace_all_inplace(output, "</Speech>", "</dd>");

    replace_all_inplace(output, "<Dialog>", R"(<div class="dialog">)");
    replace_all_inplace(output, "</Dialog>", "</div>");

    replace_all_inplace(output, "<DialogDual>",
                        R"(<div class="dialog_wrapper">)");
    replace_all_inplace(output, "</DialogDual>", "</div>");

    replace_all_inplace(output, "<DialogLeft>", R"(<dl class="first">)");
    replace_all_inplace(output, "</DialogLeft>", "</dl>");

    replace_all_inplace(output, "<DialogRight>", R"(<dl class="second">)");
    replace_all_inplace(output, "</DialogRight>", "</dl>");

    replace_all_inplace(output, "</PageBreak>", R"(<div class="page-break">)");
    replace_all_inplace(output, "</PageBreak>", "</div>");

    replace_all_inplace(output, "<Note>", R"(<p class="comment">)");
    replace_all_inplace(output, "</Note>", "</p>");

    replace_all_inplace(output, "<BlankLine>", "");
    replace_all_inplace(output, "</BlankLine>", "");

    replace_all_inplace(output, "<ActionCenter>", R"(<p class="center">)");
    replace_all_inplace(output, "</ActionCenter>", "</p>");

    static const std::regex re_newlines(R"(\n+)");
    output = std::regex_replace(output, re_newlines, "\n");

    // Unused tags:
    //   <h5 class="goldman-slugline"></h5>
    //   <span class="revised"></span>

  } catch (std::regex_error &e) {
    print_regex_error(e, __FILE__, __LINE__);
  }

  return output;
}

std::string ftn2fdx(std::string const &input) {
  std::string output{
      R"(<?xml version="1.0" encoding="UTF-8" standalone="no" ?>)"};
  output += '\n';
  output += R"(<FinalDraft DocumentType="Script" Template="No" Version="1">)";
  output += "\n<Content>\n";

  Fountain::Script script;
  script.parseFountain(input);

  output += script.to_string(Fountain::ScriptNodeType::ftnContinuation |
                             Fountain::ScriptNodeType::ftnKeyValue |
                             Fountain::ScriptNodeType::ftnSection |
                             Fountain::ScriptNodeType::ftnSynopsis |
                             Fountain::ScriptNodeType::ftnUnknown);

  output += "\n</Content>\n</FinalDraft>\n";

  try {
    replace_all_inplace(output, "<Transition>",
                        R"(<Paragraph Type="Transition"><Text>)");
    replace_all_inplace(output, "</Transition>", "</Text></Paragraph>");

    replace_all_inplace(output, "<SceneHeader>",
                        R"(<Paragraph Type="Scene Heading"><Text>)");
    replace_all_inplace(output, "</SceneHeader>", "</Text></Paragraph>");

    replace_all_inplace(output, "<Action>",
                        R"(<Paragraph Type="Action"><Text>)");
    replace_all_inplace(output, "</Action>", "</Text></Paragraph>");

    replace_all_inplace(output, "<Character>",
                        R"(<Paragraph Type="Character"><Text>)");
    replace_all_inplace(output, "</Character>", "</Text></Paragraph>");

    replace_all_inplace(output, "<Parenthetical>",
                        R"(<Paragraph Type="Parenthetical"><Text>)");
    replace_all_inplace(output, "</Parenthetical>", "</Text></Paragraph>");

    replace_all_inplace(output, "<Speech>",
                        R"(<Paragraph Type="Dialogue"><Text>)");
    replace_all_inplace(output, "</Speech>", "</Text></Paragraph>");

    replace_all_inplace(output, "<DualDialog>", "<Paragraph><DualDialog>");
    replace_all_inplace(output, "</DualDialog>", "</DualDialog></Paragraph>");

    replace_all_inplace(
        output, "<ActionCenter>",
        R"(<Paragraph Type="Action" Alignment="Center"><Text>)");
    replace_all_inplace(output, "</ActionCenter>", "</Text></Paragraph>");

    replace_all_inplace(output, "<b>", R"(<Text Style="Bold">)");
    replace_all_inplace(output, "</b>", "</Text>");

    replace_all_inplace(output, "<i>", R"(<Text Style="Italic">)");
    replace_all_inplace(output, "</i>", "</Text>");

    replace_all_inplace(output, "<u>", R"(<Text Style="Underline">)");
    replace_all_inplace(output, "</u>", "</Text>");

    replace_all_inplace(
        output, "<PageBreak>",
        R"(<Paragraph Type="Action" StartsNewPage="Yes"><Text>)");
    replace_all_inplace(output, "</PageBreak>", "</Text></Paragraph>");

    replace_all_inplace(output, "<Note>", "<ScriptNote><Text>");
    replace_all_inplace(output, "</Note>", "</Text></ScriptNote>");

    static const std::regex re_dialog("</?Dialog(Left|Right)?>");
    output = std::regex_replace(output, re_dialog, "");

    replace_all_inplace(output, "<BlankLine>", "");
    replace_all_inplace(output, "</BlankLine>", "");

    // Don't know if these work...
    replace_all_inplace(output, "<Lyric>", R"(<Paragraph Type="Lyric"><Text>)");
    replace_all_inplace(output, "</Lyric>", "</Text></Paragraph>");

    static const std::regex re_newlines(R"(\n+)");
    output = std::regex_replace(output, re_newlines, "\n");
  } catch (std::regex_error &e) {
    print_regex_error(e, __FILE__, __LINE__);
  }

  return output;
}

std::string ftn2xml(std::string const &input, std::string const &css_fn,
                    bool const &embed_css) {
  std::string output{"<!DOCTYPE html>\n<html>\n<head>\n"};

  if (!css_fn.empty()) {
    if (embed_css) {
      std::string css_contents = file_get_contents(css_fn);

      output += "<style type='text/css'>\n";
      output += css_contents;
      output += "\n</style>\n";
    } else {
      output += R"(<link rel="stylesheet" type="text/css" href=")";
      output += ((css_fn[0] == '/') ? "file://" : "") + css_fn;
      output += "'>\n";
    }
  }

  output += "</head>\n<body>\n";

  Fountain::Script script;
  script.parseFountain(input);

  output += script.to_string(Fountain::ScriptNodeType::ftnContinuation |
                             Fountain::ScriptNodeType::ftnKeyValue |
                             Fountain::ScriptNodeType::ftnUnknown);

  output += "\n</body>\n</html>\n";

  try {
    static const std::regex re_newlines(R"(\n+)");
    output = std::regex_replace(output, re_newlines, "\n");
  } catch (std::regex_error &e) {
    print_regex_error(e, __FILE__, __LINE__);
  }

  return output;
}

std::string ftn2html(std::string const &input, std::string const &css_fn,
                     bool const &embed_css) {
  std::string output{"<!DOCTYPE html>\n<html>\n<head>\n"};
  if (!css_fn.empty()) {
    if (embed_css) {
      std::string css_contents = file_get_contents(css_fn);

      output += "<style type='text/css'>\n";
      output += css_contents;
      output += "\n</style>\n";
    } else {
      output += R"(<link rel="stylesheet" type="text/css" href=")";
      output += ((css_fn[0] == '/') ? "file://" : "") + css_fn;
      output += "'>\n";
    }
  }

  output +=
      "</head>\n<body>\n"
      "<div id=\"wrapper\" class=\"fountain\">\n";

  Fountain::Script script;
  script.parseFountain(input);

  output += script.to_string(Fountain::ScriptNodeType::ftnContinuation |
                             Fountain::ScriptNodeType::ftnKeyValue |
                             Fountain::ScriptNodeType::ftnUnknown);

  output += "\n</div>\n</body>\n</html>\n";

  try {
    replace_all_inplace(output, "<Fountain>", R"(<div class="Fountain">)");
    replace_all_inplace(output, "</Fountain>", "</div>");

    replace_all_inplace(output, "<Transition>", R"(<div class="Transition">)");
    replace_all_inplace(output, "</Transition>", "</div>");

    replace_all_inplace(output, "<SceneHeader>",
                        R"(<div class="SceneHeader">)");
    replace_all_inplace(output, "</SceneHeader>", "</div>");

    replace_all_inplace(output, "<Action>", R"(<div class="Action">)");
    replace_all_inplace(output, "</Action>", "</div>");

    replace_all_inplace(output, "<Lyric>", R"(<div class="Lyric">)");
    replace_all_inplace(output, "</Lyric>", "</div>");

    replace_all_inplace(output, "<Character>", R"(<div class="Character">)");
    replace_all_inplace(output, "</Character>", "</div>");

    replace_all_inplace(output, "<Parenthetical>",
                        R"(<div class="Parenthetical">)");
    replace_all_inplace(output, "</Parenthetical>", "</div>");

    replace_all_inplace(output, "<Speech>", R"(<div class="Speech">)");
    replace_all_inplace(output, "</Speech>", "</div>");

    replace_all_inplace(output, "<Dialog>", R"(<div class="Dialog">)");
    replace_all_inplace(output, "</Dialog>", "</div>");

    replace_all_inplace(output, "<DialogDual>", R"(<div class="DialogDual">)");
    replace_all_inplace(output, "</DialogDual>", "</div>");

    replace_all_inplace(output, "<DialogLeft>", R"(<div class="DialogLeft">)");
    replace_all_inplace(output, "</DialogLeft>", "</div>");

    replace_all_inplace(output, "<DialogRight>",
                        R"(<div class="DialogRight">)");
    replace_all_inplace(output, "</DialogRight>", "</div>");

    replace_all_inplace(output, "<PageBreak>", R"(<div class="PageBreak">)");
    replace_all_inplace(output, "</PageBreak>", "</div>");

    replace_all_inplace(output, "<Note>", R"(<div class="Note">)");
    replace_all_inplace(output, "</Note>", "</div>");

    replace_all_inplace(output, "<ActionCenter>", R"(<center>)");
    replace_all_inplace(output, "</ActionCenter>", "</center>");

    replace_all_inplace(output, "<BlankLine>", "");
    replace_all_inplace(output, "</BlankLine>", "");

    for (std::size_t i = 1; i <= 6; i++) {
      std::string lvl = std::to_string(i);
      replace_all_inplace(output, "<SectionH" + lvl + ">",
                          R"(<div class="SectionH)" + lvl + R"(">)");
      replace_all_inplace(output, "</SectionH" + lvl + ">", "</div>");

      replace_all_inplace(output, "<SynopsisH" + lvl + ">",
                          R"(<div class="SynopsisH)" + lvl + R"(">)");
      replace_all_inplace(output, "</SynopsisH" + lvl + ">", "</div>");
    }

    static const std::regex re_newlines(R"(\n+)");
    output = std::regex_replace(output, re_newlines, "\n");
  } catch (std::regex_error &e) {
    print_regex_error(e, __FILE__, __LINE__);
  }

  return output;
}

#if ENABLE_EXPORT_PDF
namespace {  // PDF export

void add_char(std::string const &strAdd, std::string &strNormal,
              std::string &strBold, std::string &strItalic,
              std::string &strBoldItalic, std::string &strUnderline,
              bool const &bBold, bool const &bItalic, bool const &bUnderline) {
  if (bBold && bItalic) {
    strBoldItalic.append(strAdd);
    strBold.append(std::string(strAdd.length(), ' '));
    strItalic.append(std::string(strAdd.length(), ' '));
    strNormal.append(std::string(strAdd.length(), ' '));
  } else if (bBold) {
    strBoldItalic.append(std::string(strAdd.length(), ' '));
    strBold.append(strAdd);
    strItalic.append(std::string(strAdd.length(), ' '));
    strNormal.append(std::string(strAdd.length(), ' '));
  } else if (bItalic) {
    strBoldItalic.append(std::string(strAdd.length(), ' '));
    strBold.append(std::string(strAdd.length(), ' '));
    strItalic.append(strAdd);
    strNormal.append(std::string(strAdd.length(), ' '));
  } else {
    strBoldItalic.append(std::string(strAdd.length(), ' '));
    strBold.append(std::string(strAdd.length(), ' '));
    strItalic.append(std::string(strAdd.length(), ' '));
    strNormal.append(strAdd);
  }

  if (bUnderline) {
    strUnderline.append(std::string(strAdd.length(), '_'));
  } else {
    strUnderline.append(std::string(strAdd.length(), ' '));
  }
}

auto split_formatting(std::string const &input) {
  static bool bBold{false};
  static bool bItalic{false};
  static bool bUnderline{false};

  std::string strNormal;
  std::string strBold;
  std::string strItalic;
  std::string strBoldItalic;
  std::string strUnderline;

  if (input == "**reset**") {
    bBold = false;
    bItalic = false;
    bUnderline = false;
    return std::make_tuple(strNormal, strBold, strItalic, strBoldItalic,
                           strUnderline);
  }

  for (std::size_t pos = 0; pos < input.length(); ++pos) {
    switch (input[pos]) {
      case '<':
        if (pos + 2 < input.length()) {
          switch (input[pos + 1]) {
            case 'b':
              if (input[pos + 2] == '>') {
                bBold = true;
                pos += 2;
              } else {
                add_char(input.substr(pos, 2), strNormal, strBold, strItalic,
                         strBoldItalic, strUnderline, bBold, bItalic,
                         bUnderline);
                ++pos;
              }
              break;
            case 'i':
              if (input[pos + 2] == '>') {
                bItalic = true;
                pos += 2;
              } else {
                add_char(input.substr(pos, 2), strNormal, strBold, strItalic,
                         strBoldItalic, strUnderline, bBold, bItalic,
                         bUnderline);
                ++pos;
              }
              break;
            case 'u':
              if (input[pos + 2] == '>') {
                bUnderline = true;
                pos += 2;
              } else {
                add_char(input.substr(pos, 2), strNormal, strBold, strItalic,
                         strBoldItalic, strUnderline, bBold, bItalic,
                         bUnderline);
                ++pos;
              }
              break;
            case '/':
              if (pos + 3 < input.length()) {
                switch (input[pos + 2]) {
                  case 'b':
                    if (input[pos + 3] == '>') {
                      bBold = false;
                      pos += 3;
                    } else {
                      add_char(input.substr(pos, 3), strNormal, strBold,
                               strItalic, strBoldItalic, strUnderline, bBold,
                               bItalic, bUnderline);
                      pos += 2;
                    }
                    break;
                  case 'i':
                    if (input[pos + 3] == '>') {
                      bItalic = false;
                      pos += 3;
                    } else {
                      add_char(input.substr(pos, 3), strNormal, strBold,
                               strItalic, strBoldItalic, strUnderline, bBold,
                               bItalic, bUnderline);
                      pos += 2;
                    }
                    break;
                  case 'u':
                    if (input[pos + 3] == '>') {
                      bUnderline = false;
                      pos += 3;
                    } else {
                      add_char(input.substr(pos, 3), strNormal, strBold,
                               strItalic, strBoldItalic, strUnderline, bBold,
                               bItalic, bUnderline);
                      pos += 2;
                    }
                    break;
                  default:
                    add_char(input.substr(pos, 3), strNormal, strBold,
                             strItalic, strBoldItalic, strUnderline, bBold,
                             bItalic, bUnderline);
                    pos += 2;
                    break;
                }
              }
              break;
            default:
              add_char(input.substr(pos, 2), strNormal, strBold, strItalic,
                       strBoldItalic, strUnderline, bBold, bItalic, bUnderline);
              ++pos;
              break;
          }
        } else {
          add_char(std::string{input[pos]}, strNormal, strBold, strItalic,
                   strBoldItalic, strUnderline, bBold, bItalic, bUnderline);
        }
        break;
      default:
        add_char(std::string{input[pos]}, strNormal, strBold, strItalic,
                 strBoldItalic, strUnderline, bBold, bItalic, bUnderline);
        break;
    }
  }

  return std::make_tuple(strNormal, strBold, strItalic, strBoldItalic,
                         strUnderline);
}

std::string &center_text_inplace(std::string &text,
                                 int const line_length = 60) {
  text = std::string(int((line_length - text.length() + 1) / 2.), ' ') + text;
  return text;
}

std::vector<std::string> wrap_text(std::string const &text, int const width) {
  std::vector<std::string> words = split_string(ws_rtrim(text), " ");
  std::vector<std::string> lines;
  const std::size_t cwidth = (width * 10) / 72;

  std::string ln;
  ln.reserve(cwidth + 100);
  std::size_t len = 0;

  for (auto &w : words) {
    if (len + w.size() > cwidth) {
      lines.push_back(ws_rtrim(ln));
      ln.clear();
      len = 0;
    }

    if (len + w.size() == cwidth) {
      ln.append(w);
      lines.push_back(ws_rtrim(ln));
      ln.clear();
      len = 0;
    } else {
      ln.append(w);
      ln.append(" ");
      len += w.size() + 1;
    }
  }

  // one more line in buffer
  if (!ws_rtrim(ln).empty()) {
    lines.push_back(ws_rtrim(ln));
  }
  return lines;
}

std::vector<std::string> pdfTextLines(std::string const &text,
                                      int const width = 432) {
  std::vector<std::string> temp = split_string(ws_rtrim(text), "\n");
  std::vector<std::string> lines;

  for (auto &i : temp) {
    for (auto &j : wrap_text(i, width)) {
      lines.push_back(j);
    }
  }

  return lines;
}

void pdfTextAdd(PoDoFo::PdfStreamedDocument &document,
                PoDoFo::PdfPainter &painter, std::string const &text,
                std::string const &font, int line, int const width = 432,
                int const left_margin = 108, int const bottom_margin = 72,
                const int print_height = 648,
                PoDoFo::EPdfAlignment eAlignment = PoDoFo::ePdfAlignment_Left) {
  if (font == "title") {
    PoDoFo::PdfFont *pFontCustom = document.CreateFont("Alegreya SC Medium");
    pFontCustom->SetFontSize(24.0);
    painter.SetFont(pFontCustom);
    painter.DrawMultiLineText(
        left_margin, bottom_margin, width, print_height - 12 * line,
        PoDoFo::PdfString((const PoDoFo::pdf_utf8 *)text.c_str()), eAlignment,
        PoDoFo::ePdfVerticalAlignment_Top);
    return;
  }

  PoDoFo::PdfFont *pFontNormal = nullptr;
  if (font == "bold" || font == "sceneheader") {
    pFontNormal = document.CreateFont(PODOFO_HPDF_FONT_COURIER_BOLD);
  } else if (font == "italic" || font == "lyric") {
    pFontNormal = document.CreateFont(PODOFO_HPDF_FONT_COURIER_OBLIQUE);
  } else {
    pFontNormal = document.CreateFont(PODOFO_HPDF_FONT_COURIER);
  }

  PoDoFo::PdfFont *pFontItalic =
      document.CreateFont(PODOFO_HPDF_FONT_COURIER_OBLIQUE);

  PoDoFo::PdfFont *pFontBold =
      document.CreateFont(PODOFO_HPDF_FONT_COURIER_BOLD);

  PoDoFo::PdfFont *pFontBoldItalic =
      document.CreateFont(PODOFO_HPDF_FONT_COURIER_BOLD_OBLIQUE);

  pFontNormal->SetFontSize(12.0);
  pFontItalic->SetFontSize(12.0);
  pFontBold->SetFontSize(12.0);
  pFontBoldItalic->SetFontSize(12.0);

  // Note: PoDoFo::PdfString is heavily overloaded.
  //       Must cast as <const PoDoFo::pdf_utf8 *>
  //       to use the right version.

  std::vector<std::string> textLines = pdfTextLines(text, width);

  split_formatting("**reset**");

  for (auto textLine : textLines) {
    std::string strTextLine = ws_rtrim(textLine);
    auto formatting = split_formatting(strTextLine);
    std::string &strNormal = std::get<0>(formatting);
    std::string &strBold = std::get<1>(formatting);
    std::string &strItalic = std::get<2>(formatting);
    std::string &strBoldItalic = std::get<3>(formatting);
    std::string &strUnderline = std::get<4>(formatting);

    painter.SetFont(pFontNormal);

    painter.DrawMultiLineText(
        left_margin, bottom_margin, width, print_height - 12 * line,
        PoDoFo::PdfString((const PoDoFo::pdf_utf8 *)strNormal.c_str()),
        eAlignment, PoDoFo::ePdfVerticalAlignment_Top);

    painter.DrawMultiLineText(
        left_margin, bottom_margin, width, print_height - 12 * line,
        PoDoFo::PdfString((const PoDoFo::pdf_utf8 *)strUnderline.c_str()),
        eAlignment, PoDoFo::ePdfVerticalAlignment_Top);

    painter.SetFont(pFontBold);
    painter.DrawMultiLineText(
        left_margin, bottom_margin, width, print_height - 12 * line,
        PoDoFo::PdfString((const PoDoFo::pdf_utf8 *)strBold.c_str()),
        eAlignment, PoDoFo::ePdfVerticalAlignment_Top);

    painter.SetFont(pFontItalic);
    painter.DrawMultiLineText(
        left_margin, bottom_margin, width, print_height - 12 * line,
        PoDoFo::PdfString((const PoDoFo::pdf_utf8 *)strItalic.c_str()),
        eAlignment, PoDoFo::ePdfVerticalAlignment_Top);

    painter.SetFont(pFontBoldItalic);
    painter.DrawMultiLineText(
        left_margin, bottom_margin, width, print_height - 12 * line,
        PoDoFo::PdfString((const PoDoFo::pdf_utf8 *)strBoldItalic.c_str()),
        eAlignment, PoDoFo::ePdfVerticalAlignment_Top);

    ++line;
  }
}

}  // namespace

bool ftn2pdf(std::string const &fn, std::string const &input) {
  const int lines_per_page = 54;
  const int line_char_length = 60;
  const int width_print = 432;
  const int width_dialog = 252;
  const int width_dialog_dual = 180;
  const int indent_character = 12;
  const int indent_parenthetical = 6;
  const int indent_character_dual = 9;
  const int indent_parenthetical_dual = 7;
  const int margin_dialog = 180;
  const int margin_dialog_left = 144;
  const int margin_dialog_right = 360;
  const int left_margin = 108;
  const int bottom_margin = 72;
  const int gap_transition = 13;
  const int gap_sceneheader = 11;

  // Set up PDF document
  PoDoFo::PdfStreamedDocument document(fn.c_str());

  PoDoFo::PdfPage *pPage = document.CreatePage(
      PoDoFo::PdfPage::CreateStandardPageSize(PoDoFo::ePdfPageSize_Letter));

  if (!pPage) {
    PODOFO_RAISE_ERROR(PoDoFo::ePdfError_InvalidHandle);
  }

  PoDoFo::PdfPainter painter;
  painter.SetPage(pPage);

  PoDoFo::PdfFont *pFontNormal = document.CreateFont(PODOFO_HPDF_FONT_COURIER);
  pFontNormal->SetFontSize(12.0);
  painter.SetFont(pFontNormal);

  // process script
  Fountain::Script script(input);

  int const flags = Fountain::ScriptNodeType::ftnContinuation |
                    Fountain::ScriptNodeType::ftnKeyValue |
                    Fountain::ScriptNodeType::ftnUnknown;

  // Title page
  if (script.metadata.find("title") != script.metadata.end()) {
    std::string strText = script.metadata["title"];
    decode_entities_inplace(strText);
    replace_all_inplace(strText, "*", "");
    replace_all_inplace(strText, "_", "");

    int line = 18 - pdfTextLines(strText, width_dialog).size();
    pdfTextAdd(document, painter, "<u>" + to_upper(strText) + "</u>", "normal",
               line, 468, 72, 72, 648, PoDoFo::ePdfAlignment_Center);
    line += pdfTextLines(strText, width_dialog).size() + 4;

    if (!script.metadata["author"].empty()) {
      if (!script.metadata["credit"].empty()) {
        strText = to_lower(script.metadata["credit"]);
        decode_entities_inplace(strText);
        pdfTextAdd(document, painter, strText, "normal", line, 468, 72, 72, 648,
                   PoDoFo::ePdfAlignment_Center);
        line += pdfTextLines(strText, width_dialog).size() + 1;
      } else {
        strText = "Written by";
        pdfTextAdd(document, painter, strText, "normal", line, 468, 72, 72, 648,
                   PoDoFo::ePdfAlignment_Center);
        line += 2;
      }

      strText = script.metadata["author"];
      decode_entities_inplace(strText);
      pdfTextAdd(document, painter, strText, "normal", line, 468, 72, 72, 648,
                 PoDoFo::ePdfAlignment_Center);
      line += pdfTextLines(strText, width_dialog).size() + 4;
    }

    if (!script.metadata["source"].empty()) {
      strText = script.metadata["source"];
      decode_entities_inplace(strText);
      pdfTextAdd(document, painter, strText, "normal", line, 468, 72, 72, 648,
                 PoDoFo::ePdfAlignment_Center);
      line += pdfTextLines(strText, width_dialog).size() + 1;
    }

    if (!script.metadata["contact"].empty()) {
      strText = script.metadata["contact"];
      decode_entities_inplace(strText);

      int text_lines = pdfTextLines(strText, width_dialog).size();
      line = text_lines < 3 ? 51 : 54 - text_lines;
      pdfTextAdd(document, painter, strText, "normal", line, 468, 72, 72, 648,
                 PoDoFo::ePdfAlignment_Left);
    } else if (!script.metadata["copyright"].empty()) {
      strText = "Copyright " + script.metadata["copyright"];
      decode_entities_inplace(strText);
      replace_all_inplace(strText, "(c)", "©");

      int text_lines = pdfTextLines(strText, width_dialog).size();
      line = text_lines < 3 ? 51 : 54 - text_lines;
      pdfTextAdd(document, painter, strText, "normal", line, 468, 72, 72, 648,
                 PoDoFo::ePdfAlignment_Left);
    }

    if (!script.metadata["notes"].empty()) {
      strText = script.metadata["notes"];
      decode_entities_inplace(strText);

      int text_lines = pdfTextLines(strText, width_dialog).size();
      line -= (text_lines + 2);
      pdfTextAdd(document, painter, strText, "normal", line, 468, 72, 72, 648,
                 PoDoFo::ePdfAlignment_Right);
    }

    painter.FinishPage();
    pPage = document.CreatePage(
        PoDoFo::PdfPage::CreateStandardPageSize(PoDoFo::ePdfPageSize_Letter));
    if (!pPage) {
      PODOFO_RAISE_ERROR(PoDoFo::ePdfError_InvalidHandle);
    }
    painter.SetPage(pPage);
  }

  static int dialog_state = 0;
  std::string output;
  std::string outputDialog;
  std::string outputDialogLeft;
  std::string outputDialogRight;

  int PageNumber = 1;
  int LineNumber = 0;
  for (auto node : script.nodes) {
    std::string buffer = decode_entities(node.value + "\n");

    switch (node.type) {
      case ScriptNodeType::ftnKeyValue:
        if (flags & node.type) {
          break;
        }
        // Not used for PDF
        break;
      case ScriptNodeType::ftnPageBreak:
        if (flags & node.type) {
          break;
        }
        LineNumber += lines_per_page;
        break;
      case ScriptNodeType::ftnBlankLine:
        if (flags & node.type) {
          break;
        }
        if (dialog_state == 1) {
          int textLines = pdfTextLines(outputDialog, width_dialog).size();
          if (LineNumber + textLines <= lines_per_page) {
            pdfTextAdd(document, painter, outputDialog, "normal", LineNumber,
                       width_dialog, margin_dialog);
            outputDialog.clear();
            dialog_state = 0;
            LineNumber += textLines;
          } else {
            LineNumber += textLines;
          }
        } else if (dialog_state == 2) {
          // Don't write DialogLeft without DialogRight
        } else if (dialog_state == 3) {
          int textLines_right =
              pdfTextLines(outputDialogRight, width_dialog_dual).size();
          int textLines_left =
              pdfTextLines(outputDialogLeft, width_dialog_dual).size();
          int textLines = std::max<int>(textLines_left, textLines_right);
          if (LineNumber + textLines <= lines_per_page) {
            pdfTextAdd(document, painter, outputDialogRight, "normal",
                       LineNumber, width_dialog_dual, margin_dialog_right);
            pdfTextAdd(document, painter, outputDialogLeft, "normal",
                       LineNumber, width_dialog_dual, margin_dialog_left);
            outputDialogLeft.clear();
            outputDialogRight.clear();
            dialog_state = 0;
            LineNumber += textLines;
          } else {
            LineNumber += textLines;
          }
        }
        ++LineNumber;
        break;
      case ScriptNodeType::ftnContinuation:
        if (flags & node.type) {
          break;
        }
        // Not used for PDF
        break;
      case ScriptNodeType::ftnSceneHeader: {
        if (flags & node.type) {
          break;
        }
        // TODO: Add scene numbers?
        int textLines = pdfTextLines(buffer).size();
        if (LineNumber + gap_sceneheader <= lines_per_page) {
          pdfTextAdd(document, painter, buffer, "sceneheader", LineNumber);
          LineNumber += textLines;
        } else {
          LineNumber += gap_sceneheader;
          output += buffer;
        }
      } break;
      case ScriptNodeType::ftnAction: {
        if (flags & node.type) {
          break;
        }
        int textLines = pdfTextLines(buffer).size();
        if (LineNumber + textLines <= lines_per_page) {
          pdfTextAdd(document, painter, buffer, "normal", LineNumber);
          LineNumber += textLines;
        } else {
          LineNumber += textLines;
          output += buffer;
        }
      } break;
      case ScriptNodeType::ftnActionCenter: {
        if (flags & node.type) {
          break;
        }
        center_text_inplace(buffer);

        int textLines = pdfTextLines(buffer).size();
        if (LineNumber + textLines <= lines_per_page) {
          pdfTextAdd(document, painter, buffer, "normal", LineNumber);
          LineNumber += textLines;
        } else {
          LineNumber += textLines;
          output += buffer;
        }
      } break;
      case ScriptNodeType::ftnTransition: {
        if (flags & node.type) {
          break;
        }
        buffer =
            std::string(line_char_length - buffer.length() + 1, ' ') + buffer;

        int textLines = pdfTextLines(buffer).size();
        if (LineNumber + gap_transition <= lines_per_page) {
          pdfTextAdd(document, painter, buffer, "normal", LineNumber);
          LineNumber += textLines;
        } else {
          LineNumber += gap_transition;
          output += buffer;
        }
      } break;
      case ScriptNodeType::ftnDialog:
        if (flags & node.type) {
          break;
        }
        dialog_state = 1;
        break;
      case ScriptNodeType::ftnDialogLeft:
        if (flags & node.type) {
          break;
        }
        dialog_state = 2;
        break;
      case ScriptNodeType::ftnDialogRight:
        if (flags & node.type) {
          break;
        }
        dialog_state = 3;
        break;

      case ScriptNodeType::ftnCharacter:
        if (flags & node.type) {
          break;
        }
        if (dialog_state == 1) {
          outputDialog += std::string(indent_character, ' ');
          outputDialog += buffer;
        } else if (dialog_state == 2) {
          outputDialogLeft += std::string(indent_character_dual, ' ');
          outputDialogLeft += buffer;
        } else if (dialog_state == 3) {
          outputDialogRight += std::string(indent_character_dual, ' ');
          outputDialogRight += buffer;
        }
        break;
      case ScriptNodeType::ftnParenthetical:
        if (flags & node.type) {
          break;
        }
        if (dialog_state == 1) {
          outputDialog += std::string(indent_parenthetical, ' ');
          outputDialog += buffer;
        } else if (dialog_state == 2) {
          outputDialogLeft += std::string(indent_parenthetical_dual, ' ');
          outputDialogLeft += buffer;
        } else if (dialog_state == 3) {
          outputDialogRight += std::string(indent_parenthetical_dual, ' ');
          outputDialogRight += buffer;
        }
        break;
      case ScriptNodeType::ftnSpeech: {
        if (flags & node.type) {
          break;
        }
        if (dialog_state == 1) {
          outputDialog += buffer;
        } else if (dialog_state == 2) {
          outputDialogLeft += buffer;
        } else if (dialog_state == 3) {
          outputDialogRight += buffer;
        }
      } break;
      case ScriptNodeType::ftnLyric: {
        if (flags & node.type) {
          break;
        }
        if (dialog_state == 1) {
          outputDialog += "<i>" + buffer + "</i>";
        } else if (dialog_state == 2) {
          outputDialogLeft += "<i>" + buffer + "</i>";
        } else if (dialog_state == 3) {
          outputDialogRight += "<i>" + buffer + "</i>";
        } else {
          int textLines = pdfTextLines(buffer).size();
          if (LineNumber + textLines <= lines_per_page) {
            pdfTextAdd(document, painter, buffer, "lyric", LineNumber);
            LineNumber += textLines;
          } else {
            LineNumber += textLines;
            output += buffer;
          }
        }
      } break;
      case ScriptNodeType::ftnNotation:
        if (flags & node.type) {
          break;
        }
        // not used for PDF
        break;
      case ScriptNodeType::ftnSection:
        if (flags & node.type) {
          break;
        }
        // Not used for PDF
        break;
      case ScriptNodeType::ftnSynopsis:
        if (flags & node.type) {
          break;
        }
        // Not used for PDF
        break;
      case ScriptNodeType::ftnUnknown:
      default:
        if (flags & node.type) {
          break;
        }
        // Not used for PDF
        break;
    }

    // Add new page
    if (LineNumber >= lines_per_page) {
      painter.FinishPage();
      LineNumber = 0;
      pPage = document.CreatePage(
          PoDoFo::PdfPage::CreateStandardPageSize(PoDoFo::ePdfPageSize_Letter));
      if (!pPage) {
        PODOFO_RAISE_ERROR(PoDoFo::ePdfError_InvalidHandle);
      }
      painter.SetPage(pPage);
      ++PageNumber;

      // Add page number
      std::string PageNumberText = std::to_string(PageNumber);
      PageNumberText =
          std::string(line_char_length - PageNumberText.length() - 1, ' ') +
          PageNumberText + '.';
      pdfTextAdd(document, painter, PageNumberText, "normal", -3);

      // Add overflow from previous page
      if (dialog_state == 1 && !outputDialog.empty()) {
        int textLines = pdfTextLines(outputDialog, width_dialog).size();
        pdfTextAdd(document, painter, outputDialog, "normal", LineNumber,
                   width_dialog, margin_dialog);
        outputDialog.clear();
        dialog_state = 0;
        LineNumber += textLines + 1;
      } else if (dialog_state == 2) {
        // Don't write DialogLeft without DialogRight
      } else if (dialog_state == 3 &&
                 (!outputDialogLeft.empty() || !outputDialogRight.empty())) {
        int textLines_left =
            pdfTextLines(outputDialogLeft, width_dialog_dual).size();
        int textLines_right =
            pdfTextLines(outputDialogRight, width_dialog_dual).size();
        int textLines = std::max<int>(textLines_left, textLines_right);

        pdfTextAdd(document, painter, outputDialogLeft, "normal", LineNumber,
                   width_dialog_dual, margin_dialog_left);
        pdfTextAdd(document, painter, outputDialogRight, "normal", LineNumber,
                   width_dialog_dual, margin_dialog_right);
        outputDialogLeft.clear();
        outputDialogRight.clear();
        dialog_state = 0;
        LineNumber += textLines + 1;
      } else if (!output.empty()) {
        if (node.type == ScriptNodeType::ftnSceneHeader) {
          int textLines = pdfTextLines(output).size();
          pdfTextAdd(document, painter, output, "sceneheader", LineNumber);
          output.clear();
          LineNumber += textLines;
        } else {
          int textLines = pdfTextLines(output).size();
          pdfTextAdd(document, painter, output, "normal", LineNumber);
          output.clear();
          LineNumber += textLines;
        }
      }
    }
  }

  painter.FinishPage();
  document.GetInfo()->SetCreator("Geany Preview Plugin");

  if (!script.metadata["author"].empty()) {
    document.GetInfo()->SetAuthor(script.metadata["author"]);
  }

  if (!script.metadata["title"].empty()) {
    document.GetInfo()->SetTitle(script.metadata["title"]);
  }

  document.Close();
  return true;
}
#endif  // ENABLE_EXPORT_PDF

}  // namespace Fountain
