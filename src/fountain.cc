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

#include <string.h>

#include "auxiliary.h"
#include "fountain.h"

namespace Fountain {

namespace {

bool isForced(std::string const &input) {
  if (input.length() < 1) {
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
  if (input.length() < 1) {
    return false;
  }

  // Note: input should already be left-trimmed,
  // otherwise indent would be removed here

  // forced transition; make sure intent is not to center
  size_t len = input.length();
  if (input[0] == '>' && input[len - 1] != '<') {
    return true;
  }

  if (isForced(input)) {
    return false;
  }

  // must be uppercase
  if (!is_upper(input)) {
    return false;
  }

  // Standard transitions ending with, " TO:"
  //   "CUT TO:",      "DISSOLVE TO:",  "FADE TO:",           "FLASH CUT TO:",
  //   "JUMP CUT TO:", "MATCH CUT TO:", "MATCH DISSOLVE TO:", "SMASH CUT TO:",
  //   "WIPE TO:",
  if (len < 4) {
    return false;
  }
  std::string end = input.substr(len - 4);
  if (end == std::string(" TO:")) {
    return true;
  }

  // Other transitions
  const char *transitions[] = {
      "CUT TO BLACK:", "DISSOLVE:",        "END CREDITS:",   "FADE IN:",
      "FADE OUT.",     "FREEZE FRAME:",    "INTERCUT WITH:", "IRIS IN:",
      "IRIS OUT.",     "OPENING CREDITS:", "SPLIT SCREEN:",  "STOCK SHOT:",
      "TIME CUT:",     "TITLE OVER:",      "WIPE:",
  };

  for (auto str : transitions) {
    if (input == std::string(str)) {
      return true;
    }
  }

  return false;
}

std::string parseTransition(std::string const &input) {
  std::string output;
  if (input[0] == '>') {
    output = to_upper(input.substr(1));
  } else {
    output = to_upper(input);
  }
  return trim_inplace(output);
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
        R"(^(INT|EXT|EST|INT\.?/EXT|EXT\.?/INT|I/E)[\.\ ])",
        std::regex_constants::icase);

    if (std::regex_search(input, re_scene_header)) {
      return true;
    }
  } catch (std::regex_error &e) {
    printf("regex error in isSceneHeader\n");
    print_regex_error(e);
  }
  return false;
}

// returns <scene description, scene number>
std::pair<std::string, std::string> parseSceneHeader(std::string const &input) {
  std::string first;
  std::string second;
  bool forced_scene{false};

  if (input[0] == '.' && input[1] != '.') {
    forced_scene = true;
  }

  long pos = input.find("#");
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
  static std::string strWhiteSpace{FOUNTAIN_WHITESPACE};
  if (strWhiteSpace.find(input.back()) != std::string::npos) {
    std::string s = ws_rtrim(input);
    const size_t len = s.length();
    if (s[len - 1] == ']' && s[len - 2] == ']') {
      return true;
    }
  } else {
    const size_t len = input.length();
    if (input[len - 1] == ']' && input[len - 2] == ']') {
      return true;
    }
  }

  return false;
}

bool isCharacter(std::string const &input) {
  if (input.length() < 1) {
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

  size_t pos = input.find("(");
  if (pos != std::string::npos && input.find(")") != std::string::npos) {
    if (is_upper(input.substr(0, pos))) {
      return true;
    }
  } else if (is_upper(input)) {
    return true;
  }

  return false;
}

std::string parseCharacter(std::string const &input) {
  // Note: input should already be left-trimmed,
  // otherwise indent would be removed here
  std::string output;

  if (input[0] == '@') {
    output = ws_ltrim(input.substr(1));
  } else {
    output = input;
  }

  const size_t len = output.length();

  if (output[len - 1] == '^') {
    output = ws_rtrim(output.substr(0, len - 1));
  }

  return output;
}

bool isDualDialog(std::string const &input) {
  size_t len = input.length();

  if (len > 0 && input[len - 1] == '^') {
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
    const size_t len = s.length();
    if (s[len - 1] == ')') {
      return true;
    }
  } else {
    const size_t len = input.length();
    if (input[len - 1] == ')') {
      return true;
    }
  }
  return false;
}

bool isContinuation(std::string const &input) {
  if (input.length() < 1) {
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

  size_t pos = input.find(':');

  if (pos != std::string::npos) {
    key = ws_trim(input.substr(0, pos));
    value = ws_trim(input.substr(pos + 1));
  }

  return std::make_pair(key, value);
}

}  // namespace

std::string ScriptNode::to_string(size_t const &flags) const {
  static uint8_t dialog_state = 0;
  std::string output;

  switch (type) {
    case ScriptNodeType::ftnKeyValue:
      if (flags & type) {
        break;
      }
      output += "<meta>\n<key>" + key + "</key>\n<value>" + value +
                "</value>\n</meta>";
      break;
    case ScriptNodeType::ftnPageBreak:
      if (flags & type) {
        break;
      }
      if (dialog_state) {
        dialog_state == 1   ? output += "</Dialog>\n"
        : dialog_state == 2 ? output += "</DialogLeft>\n"
                            : output += "</DialogRight>\n</DualDialog>\n";
        dialog_state = 0;
      }
      output += "<PageBreak></PageBreak>";
      break;
    case ScriptNodeType::ftnBlankLine:
      if (flags & type) {
        break;
      }
      if (dialog_state) {
        dialog_state == 1   ? output += "</Dialog>\n"
        : dialog_state == 2 ? output += "</DialogLeft>\n"
                            : output += "</DialogRight>\n</DualDialog>\n";
        dialog_state = 0;
      }
      output += "<BlankLine></BlankLine>";
      break;
    case ScriptNodeType::ftnContinuation:
      if (flags & type) {
        break;
      }
      output += "<Continuation>" + value + "</Continuation>";
      break;
    case ScriptNodeType::ftnSceneHeader:
      if (flags & type) {
        break;
      }
      if (!key.empty()) {
        output += "<SceneHeader><SceneNumL>" + key + "</SceneNumL>" + value +
                  +"<SceneNumR>" + key + "</SceneNumR></SceneHeader>";
      } else {
        output += "<SceneHeader>" + value + "</SceneHeader>";
      }
      break;
    case ScriptNodeType::ftnAction:
      if (flags & type) {
        break;
      }
      output += "<Action>" + value + "</Action>";
      break;
    case ScriptNodeType::ftnTransition:
      if (flags & type) {
        break;
      }
      output += "<Transition>" + value + "</Transition>";
      break;
    case ScriptNodeType::ftnDialog:
      if (flags & type) {
        break;
      }
      dialog_state = 1;
      output += "<Dialog>" + key;
      break;
    case ScriptNodeType::ftnDialogLeft:
      if (flags & type) {
        break;
      }
      dialog_state = 2;
      output += "<DualDialog>\n<DialogLeft>" + value;
      break;
    case ScriptNodeType::ftnDialogRight:
      if (flags & type) {
        break;
      }
      dialog_state = 3;
      output += "<DialogRight>" + value;
      break;
    case ScriptNodeType::ftnCharacter:
      if (flags & type) {
        break;
      }
      output += "<Character>" + value + "</Character>";
      break;
    case ScriptNodeType::ftnParenthetical:
      if (flags & type) {
        break;
      }
      output += "<Parenthetical>" + value + "</Parenthetical>";
      break;
    case ScriptNodeType::ftnSpeech:
      if (flags & type) {
        break;
      }
      output += "<Speech>" + value + "</Speech>";
      break;
    case ScriptNodeType::ftnNotation:
      if (flags & type) {
        break;
      }
      output += "<Note>" + value + "</Note>";
      break;
    case ScriptNodeType::ftnLyric:
      if (flags & type) {
        break;
      }
      output += "<Lyric>" + value + "</Lyric>";
      break;
    case ScriptNodeType::ftnSection:
      if (flags & type) {
        break;
      }
      output += "<SectionH" + key + ">" + value + "</SectionH" + key + ">";
      break;
    case ScriptNodeType::ftnSynopsis:
      if (flags & type) {
        break;
      }
      output += "<SynopsisH" + key + ">" + value + "</SynopsisH" + key + ">";
      break;
    case ScriptNodeType::ftnUnknown:
    default:
      if (flags & type) {
        break;
      }
      output += "<Unknown>" + value + "</Unknown>";
      break;
  }
  return output;
}

std::string Script::to_string(size_t const &flags) const {
  std::string output{"<Fountain>\n"};

  for (auto node : nodes) {
    output += node.to_string(flags) + "\n";
  }

  output += "\n</Fountain>";
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
    printf("regex error in parseNodeText\n");
    print_regex_error(e);
    return input;
  }
}

void Script::parseFountain(std::string const &text) {
  if (!text.length()) {
    return;
  }

  std::vector<std::string> lines;
  try {
    // remove comments
    static const std::regex re_comment(R"(/\*[\S\s]*?\*/)");
    std::string strTmp = std::regex_replace(text, re_comment, "");

    // TODO: Perform escape replacements in a single pass?
    // simple tab and escape sequence replacements
    replace_all_inplace(strTmp, "\t", "    ");
    replace_all_inplace(strTmp, R"(\&)", "&#38;");
    replace_all_inplace(strTmp, R"(\*)", "&#42;");
    replace_all_inplace(strTmp, R"(\_)", "&#95;");
    replace_all_inplace(strTmp, R"(\:)", "&#58;");
    replace_all_inplace(strTmp, R"(\[)", "&#91;");
    replace_all_inplace(strTmp, R"(\])", "&#93;");
    replace_all_inplace(strTmp, R"(\\)", "&#92;");
    replace_all_inplace(strTmp, R"(\<)", "&#60;");
    replace_all_inplace(strTmp, R"(\>)", "&#62;");
    replace_all_inplace(strTmp, R"(\.)", "&#46;");

    // replace non-entity ampersands
    static const std::regex re_ampersand("&(?!#?[a-zA-Z0-9]+;)");
    strTmp = std::regex_replace(strTmp, re_ampersand, "&#38;");

    lines = split_lines(strTmp);
  } catch (std::regex_error &e) {
    printf("regex error in parseFountain\n");
    print_regex_error(e);
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
    printf("regex error in header check\n");
    print_regex_error(e);
  }

  // used for synposis
  int currSection = 1;

  // process line-by-line
  for (auto &line : lines) {
    std::string s = ws_ltrim(line);

    if (has_header) {
      if (s.find(':') != std::string::npos) {
        auto kv = parseKeyValue(s);
        new_node(ScriptNodeType::ftnKeyValue, kv.first);
        append(kv.second);
        continue;
      }
      if (line.length() == 0) {
        has_header = false;
      } else {
        append(ws_rtrim(s));
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

    // Character
    if (curr_node.type == ScriptNodeType::ftnUnknown && isCharacter(s)) {
      if (isDualDialog(s)) {
        // modify previous dialog node
        bool prev_dialog_modded = false;
        for (int64_t pos = nodes.size() - 1; pos >= 0; pos--) {
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

    // Speech
    if (curr_node.type == ScriptNodeType::ftnSpeech) {
      append(s);
      continue;
    } else if (!nodes.empty()) {
      ScriptNodeType ct = nodes.back().type;
      if (ct == ScriptNodeType::ftnParenthetical ||
          ct == ScriptNodeType::ftnCharacter) {
        new_node(ScriptNodeType::ftnSpeech);
        append(s);
        continue;
      }
    }

    // Section, can only be forced
    if (s.length() > 0 && s[0] == '#') {
      for (uint8_t i = 1; i < 6; i++) {
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

    // Lyric, can only be forced
    if (s.length() > 1 && s[0] == '~') {
      new_node(ScriptNodeType::ftnLyric);
      append(s.substr(1));
      end_node();
      continue;
    }

    // Probably Action
    // Note: Leading whitespace is retained in Action
    if (curr_node.type == ScriptNodeType::ftnAction) {
      if (isCenter(s)) {
        append("<center>" + ws_trim(s.substr(1, s.length() - 2)) + "</center>");
      } else {
        append(line);
      }
      continue;
    }
    if (isCenter(s)) {
      new_node(ScriptNodeType::ftnAction);
      append("<center>" + ws_trim(s.substr(1, s.length() - 2)) + "</center>");
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
std::string ftn2screenplain(std::string const &input,
                            std::string const &css_fn) {
  std::string output{"<!DOCTYPE html>\n<html>\n<head>\n"};

  if (!css_fn.empty()) {
    output += R"(<link rel="stylesheet" type="text/css" href="file://)";
    output += css_fn;
    output += "\">\n";
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

    replace_all_inplace(output, "<BlankLine>", "");
    replace_all_inplace(output, "</BlankLine>", "");

    static const std::regex re_newlines(R"(\n+)");
    output = std::regex_replace(output, re_newlines, "\n");
  } catch (std::regex_error &e) {
    printf("regex error in ftn2screenplain\n");
    print_regex_error(e);
  }

  return output;
}

// html similar to textplay html output (can use the same css files)
std::string ftn2textplay(std::string const &input, std::string const &css_fn) {
  std::string output{"<!DOCTYPE html>\n<html>\n<head>\n"};

  if (!css_fn.empty()) {
    output += "<link rel=\"stylesheet\" type=\"text/css\" href=\"file://";
    output += css_fn;
    output += "\">\n";
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

    replace_all_inplace(output, "<center>", R"(<p class="center">)");
    replace_all_inplace(output, "</center>", "</p>");

    static const std::regex re_newlines(R"(\n+)");
    output = std::regex_replace(output, re_newlines, "\n");

    // Unused tags:
    //   <h5 class="goldman-slugline"></h5>
    //   <span class="revised"></span>

  } catch (std::regex_error &e) {
    printf("regex error in ftn2textplay\n");
    print_regex_error(e);
  }

  return output;
}

std::string ftn2fdx(std::string const &input) {
  std::string output{
      R"(<?xml version="1.0" encoding="UTF-8" standalone="no" ?>)"};
  output += "\n";
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
        output, "<center>",
        R"(<Paragraph Type="Action" Alignment="Center"><Text>)");
    replace_all_inplace(output, "</center>", "</Text></Paragraph>");

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
    printf("regex error in ftn2fdx\n");
    print_regex_error(e);
  }

  return output;
}

std::string ftn2xml(std::string const &input, std::string const &css_fn) {
  std::string output{"<!DOCTYPE html>\n<html>\n<head>\n"};

  if (!css_fn.empty()) {
    output += "<link rel=\"stylesheet\" type=\"text/css\" href=\"file://";
    output += css_fn;
    output += "\">\n";
  }

  output +=
      "</head>\n<body>\n"
      "<Fountain>\n";

  Fountain::Script script;
  script.parseFountain(input);

  output += script.to_string(Fountain::ScriptNodeType::ftnContinuation |
                             Fountain::ScriptNodeType::ftnKeyValue |
                             Fountain::ScriptNodeType::ftnUnknown);

  output += "\n</Fountain>\n</body>\n</html>\n";

  try {
    replace_all_inplace(output, "<BlankLine></BlankLine>", "");

    static const std::regex re_newlines(R"(\n+)");
    output = std::regex_replace(output, re_newlines, "\n");
  } catch (std::regex_error &e) {
    printf("regex error in ftn2xml\n");
    print_regex_error(e);
  }

  return output;
}

}  // namespace Fountain
