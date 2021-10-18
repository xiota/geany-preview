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

#include <regex>

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
  if (!input.length()) {
    return false;
  }

  // ignore indent
  std::string s = ws_ltrim(input);
  size_t len = s.length();

  // forced transition
  if (len > 0 && s[0] == '>' && s[len - 1] != '<') {
    return true;
  }

  if (isForced(s)) {
    return false;
  }

  // must be uppercase
  if (!is_upper(s)) {
    return false;
  }

  // Standard transitions ending with, " TO:"
  //   "CUT TO:",      "DISSOLVE TO:",  "FADE TO:",           "FLASH CUT TO:",
  //   "JUMP CUT TO:", "MATCH CUT TO:", "MATCH DISSOLVE TO:", "SMASH CUT TO:",
  //   "WIPE TO:",
  if (s.length() < 4) {
    return false;
  }
  std::string end = s.substr(len - 4);
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
    if (s == std::string(str)) {
      return true;
    }
  }

  return false;
}

std::string parseTransition(std::string const &input) {
  std::string r = ws_trim(input);
  if (r[0] == '>') {
    r = r.substr(1);
  }

  return to_upper(ws_ltrim(r));
}

bool isSceneHeader(std::string const &input) {
  const std::string s = ws_ltrim(input);

  if (s.length() < 2) {
    return false;
  }

  if (s[0] == '.' && s[1] != '.') {
    return true;
  }

  if (isForced(s)) {
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

std::pair<std::string, std::string> parseSceneHeader(std::string const &input) {
  std::string first;
  std::string second;

  long pos = input.find("#");
  if (pos < input.length() - 1 && input.back() == '#') {
    first = ws_trim(input.substr(0, pos));
    second = input.substr(pos + 1);
    second.pop_back();
  } else {
    first = ws_trim(input);
  }

  if (first[0] == '.' && first[1] != '.') {
    return std::make_pair(ws_ltrim(first.substr(1)), trim(second));
  } else {
    return std::make_pair(first, trim(second));
  }
}

bool isCenter(std::string const &input) {
  if (input.length() < 2) {
    return false;
  }

  // ignore indent
  std::string s = ws_ltrim(input);
  size_t len = s.length();

  if (len > 1 && s[0] == '>' && s[len - 1] == '<') {
    return true;
  }

  return false;
}

bool isNotation(std::string const &input) {
  if (input.length() < 4) {
    return false;
  }

  std::string s = ws_trim(input);
  size_t len = s.length();

  if (len > 3 && s[0] == '[' && s[1] == '[' && s[len - 1] == ']' &&
      s[len - 2] == ']') {
    return true;
  }

  return false;
}

bool isCharacter(std::string const &input) {
  if (!input.length()) {
    return false;
  }

  // ignore indent
  std::string s = ws_ltrim(input);
  size_t len = s.length();

  // forced character
  if (len > 0 && s[0] == '@') {
    return true;
  }

  if (isForced(s)) {
    return false;
  }

  if (input.find("(") != std::string::npos ||
      input.find(")") != std::string::npos) {
    size_t pos = s.find("(");
    if (is_upper(s.substr(0, pos))) {
      return true;
    }
  } else if (is_upper(s)) {
    return true;
  }

  return false;
}

std::string parseCharacter(std::string const &input) {
  std::string r = ws_ltrim(input);
  if (r[0] == '>') {
    r = ws_ltrim(r.substr(1));
  }
  if (r[r.length() - 1] == '^') {
    r = ws_rtrim(r.substr(0, r.length() - 1));
  }
  return to_upper(r);
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

  // ignore surrounding spaces
  std::string s = ws_trim(input);
  size_t len = s.length();

  if (len > 0 && s[0] == '(' && s[len - 1] == ')') {
    return true;
  }

  return false;
}

bool isContinuation(std::string const &input) {
  try {
    static const std::regex re_continuation(R"(^\s*[\s\.]$)");

    if (std::regex_search(input, re_continuation)) {
      return true;
    }
  } catch (std::regex_error &e) {
    printf("regex error in isContinuation\n");
    print_regex_error(e);
  }
  return false;
}

auto parseKeyValue(std::string const &input) {
  std::string s = ws_trim(input);
  std::string key;
  std::string value;

  size_t pos = s.find(':');

  if (pos != std::string::npos) {
    key = s.substr(0, pos);
    value = s.substr(pos + 1);
  }

  return std::make_pair(key, ws_ltrim(value));
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

  // remove comments, replace tabs
  try {
    static const std::regex re_comment(R"(/\*[\S\s]*?\*/)");
    static const std::regex re_tabs(R"(\t)");
    std::string strTmp = std::regex_replace(text, re_comment, "");
    strTmp = std::regex_replace(strTmp, re_tabs, "    ");

    static const std::regex re_ampersand(R"(&(^=#?[a-zA-Z0-9]+;))");
    static const std::regex re_asterisk(R"(\\\*)");
    static const std::regex re_underscore(R"(\\_)");
    static const std::regex re_colon(R"(\\:)");
    strTmp = std::regex_replace(strTmp, re_ampersand, R"(&#38;)");
    strTmp = std::regex_replace(strTmp, re_asterisk, R"(&#42;)");
    strTmp = std::regex_replace(strTmp, re_underscore, R"(&#95;)");
    strTmp = std::regex_replace(strTmp, re_colon, R"(&#58;)");

    static const std::regex re_bracket_left(R"(\\\[)");
    static const std::regex re_bracket_right(R"(\\\])");
    static const std::regex re_backslash(R"(\\\\)");
    static const std::regex re_lt(R"(\\<)");
    static const std::regex re_gt(R"(\\>)");
    strTmp = std::regex_replace(strTmp, re_bracket_left, R"(&#91;)");
    strTmp = std::regex_replace(strTmp, re_bracket_right, R"(&#93;)");
    strTmp = std::regex_replace(strTmp, re_backslash, R"(&#92;)");
    strTmp = std::regex_replace(strTmp, re_lt, R"(&#60;)");
    strTmp = std::regex_replace(strTmp, re_gt, R"(&#62;)");

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
        append(s);
      } else {
        append(s);
        new_node(ScriptNodeType::ftnBlankLine, "");
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
      new_node(ScriptNodeType::ftnTransition, "");
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
          new_node(ScriptNodeType::ftnDialogRight, "");
        } else {
          new_node(ScriptNodeType::ftnDialog, "");
        }

        new_node(ScriptNodeType::ftnCharacter, "");
        append(parseCharacter(s));
        end_node();
      } else {
        new_node(ScriptNodeType::ftnDialog, "");
        new_node(ScriptNodeType::ftnCharacter, "");
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
          new_node(ScriptNodeType::ftnParenthetical, "");
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
        new_node(ScriptNodeType::ftnSpeech, "");
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
      append(ws_ltrim(s.substr(1)));
      end_node();
      continue;
    }

    // Lyric, can only be forced
    if (s.length() > 1 && s[0] == '~') {
      new_node(ScriptNodeType::ftnLyric, "");
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
      new_node(ScriptNodeType::ftnAction, "");
      append("<center>" + ws_trim(s.substr(1, s.length() - 2)) + "</center>");
      continue;
    }
    if (s.length() > 1 && s[0] == '!') {
      new_node(ScriptNodeType::ftnAction, "");
      append(s.substr(1));
      continue;
    }
    if (curr_node.type == ScriptNodeType::ftnUnknown) {
      new_node(ScriptNodeType::ftnAction, "");
      append(line);
      continue;
    }
  }

  end_node();
}

}  // namespace Fountain

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
    static const std::regex re_transition1("<Transition>");
    output = std::regex_replace(output, re_transition1,
                                R"(<div class="transition">)");

    static const std::regex re_transition2("</Transition>");
    output = std::regex_replace(output, re_transition2, "</div>");

    static const std::regex re_sceneheader1("<SceneHeader>");
    output = std::regex_replace(output, re_sceneheader1,
                                R"(<h6 class="sceneheader">)");

    static const std::regex re_sceneheader2("</SceneHeader>");
    output = std::regex_replace(output, re_sceneheader2, "</h6>");

    static const std::regex re_action1("<Action>");
    output = std::regex_replace(output, re_action1, R"(<div class="action">)");

    static const std::regex re_action2("</Action>");
    output = std::regex_replace(output, re_action2, "</div>");

    static const std::regex re_lyric1("<Lyric>");
    output = std::regex_replace(output, re_lyric1, R"(<div class="lyric">)");

    static const std::regex re_lyric2("</Lyric>");
    output = std::regex_replace(output, re_lyric2, "</div>");

    static const std::regex re_character1("<Character>");
    output =
        std::regex_replace(output, re_character1, R"(<p class="character">)");

    static const std::regex re_character2("</Character>");
    output = std::regex_replace(output, re_character2, "</p>");

    static const std::regex re_parenthetical1("<Parenthetical>");
    output = std::regex_replace(output, re_parenthetical1,
                                R"(<p class="parenthetical">)");

    static const std::regex re_parenthetical2("</Parenthetical>");
    output = std::regex_replace(output, re_parenthetical2, "</p>");

    static const std::regex re_speech1("<Speech>");
    output = std::regex_replace(output, re_speech1, R"(<p class="speech">)");

    static const std::regex re_speech2("</Speech>");
    output = std::regex_replace(output, re_speech2, "</p>");

    static const std::regex re_dialog1("<Dialog>");
    output = std::regex_replace(output, re_dialog1, R"(<div class="dialog">)");

    static const std::regex re_dialog2("</Dialog>");
    output = std::regex_replace(output, re_dialog2, "</div>");

    static const std::regex re_dialogdual1("<DialogDual>");
    output =
        std::regex_replace(output, re_dialogdual1, R"(<div class="dual">)");

    static const std::regex re_dialogdual2("</DialogDual>");
    output = std::regex_replace(output, re_dialogdual2, "</div>");

    static const std::regex re_dialogleft1("<DialogLeft>");
    output =
        std::regex_replace(output, re_dialogleft1, R"(<div class="left">)");

    static const std::regex re_dialogleft2("</DialogLeft>");
    output = std::regex_replace(output, re_dialogleft2, "</div>");

    static const std::regex re_dialogright1("<DialogRight>");
    output =
        std::regex_replace(output, re_dialogright1, R"(<div class="right">)");

    static const std::regex re_dialogright2("</DialogRight>");
    output = std::regex_replace(output, re_dialogright2, "</div>");

    static const std::regex re_pagebreak1("<PageBreak>");
    output = std::regex_replace(output, re_pagebreak1,
                                R"(<div class="page-break">)");

    static const std::regex re_pagebreak2("</PageBreak>");
    output = std::regex_replace(output, re_pagebreak2, "</div>");

    static const std::regex re_note1("<Note>");
    output = std::regex_replace(output, re_note1, R"(<div class="note">)");

    static const std::regex re_note2("</Note>");
    output = std::regex_replace(output, re_note2, "</div>");

    static const std::regex re_blankline("</?BlankLine>");
    output = std::regex_replace(output, re_blankline, "");

    static const std::regex re_continuation("</?Continuation>");
    output = std::regex_replace(output, re_continuation, "");

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
    static const std::regex re_transition1("<Transition>");
    output = std::regex_replace(output, re_transition1,
                                R"(<h3 class="right-transition">)");

    static const std::regex re_transition2("</Transition>");
    output = std::regex_replace(output, re_transition2, "</h3>");

    static const std::regex re_sceneheader1("<SceneHeader>");
    output = std::regex_replace(output, re_sceneheader1,
                                R"(<h2 class="full-slugline">)");

    static const std::regex re_sceneheader2("</SceneHeader>");
    output = std::regex_replace(output, re_sceneheader2, "</h2>");

    static const std::regex re_action1("<Action>");
    output = std::regex_replace(output, re_action1, R"(<p class="action">)");

    static const std::regex re_action2("</Action>");
    output = std::regex_replace(output, re_action2, "</p>");

    static const std::regex re_lyric1("<Lyric>");
    output = std::regex_replace(output, re_lyric1, R"(<span class="lyric">)");

    static const std::regex re_lyric2("</Lyric>");
    output = std::regex_replace(output, re_lyric2, "</span>");

    static const std::regex re_character1("<Character>");
    output =
        std::regex_replace(output, re_character1, R"(<dt class="character">)");

    static const std::regex re_character2("</Character>");
    output = std::regex_replace(output, re_character2, "</dt>");

    static const std::regex re_parenthetical1("<Parenthetical>");
    output = std::regex_replace(output, re_parenthetical1,
                                R"(<dd class="parenthetical">)");

    static const std::regex re_parenthetical2("</Parenthetical>");
    output = std::regex_replace(output, re_parenthetical2, "</dd>");

    static const std::regex re_speech1("<Speech>");
    output = std::regex_replace(output, re_speech1, R"(<dd class="dialogue">)");

    static const std::regex re_speech2("</Speech>");
    output = std::regex_replace(output, re_speech2, "</dd>");

    static const std::regex re_dialog1("<Dialog>");
    output = std::regex_replace(output, re_dialog1, R"(<div class="dialog">)");

    static const std::regex re_dialog2("</Dialog>");
    output = std::regex_replace(output, re_dialog2, "</div>");

    static const std::regex re_dialogdual1("<DialogDual>");
    output = std::regex_replace(output, re_dialogdual1,
                                R"(<div class="dialog_wrapper">)");

    static const std::regex re_dialogdual2("</DialogDual>");
    output = std::regex_replace(output, re_dialogdual2, "</div>");

    static const std::regex re_dialogleft1("<DialogLeft>");
    output =
        std::regex_replace(output, re_dialogleft1, R"(<dl class="first">)");

    static const std::regex re_dialogleft2("</DialogLeft>");
    output = std::regex_replace(output, re_dialogleft2, "</dl>");

    static const std::regex re_dialogright1("<DialogRight>");
    output =
        std::regex_replace(output, re_dialogright1, R"(<dl class="second">)");

    static const std::regex re_dialogright2("</DialogRight>");
    output = std::regex_replace(output, re_dialogright2, "</dl>");

    static const std::regex re_pagebreak1("</PageBreak>");
    output = std::regex_replace(output, re_pagebreak1,
                                R"(<div class="page-break">)");

    static const std::regex re_pagebreak2("</PageBreak>");
    output = std::regex_replace(output, re_pagebreak2, "</div>");

    static const std::regex re_note1("<Note>");
    output = std::regex_replace(output, re_note1, R"(<p class="comment">)");

    static const std::regex re_note2("</Note>");
    output = std::regex_replace(output, re_note2, "</p>");

    static const std::regex re_blankline("</?BlankLine>");
    output = std::regex_replace(output, re_blankline, "");

    static const std::regex re_continuation("</?Continuation>");
    output = std::regex_replace(output, re_continuation, "");

    static const std::regex re_center1("<center>");
    output = std::regex_replace(output, re_center1, R"(<p class="center">)");

    static const std::regex re_center2("</center>");
    output = std::regex_replace(output, re_center2, "</p>");

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
    static const std::regex re_transition1("<Transition>");
    output = std::regex_replace(output, re_transition1,
                                R"(<Paragraph Type="Transition"><Text>)");

    static const std::regex re_transition2("</Transition>");
    output = std::regex_replace(output, re_transition2, "</Text></Paragraph>");

    static const std::regex re_sceneheader1("<SceneHeader>");
    output = std::regex_replace(output, re_sceneheader1,
                                R"(<Paragraph Type="Scene Heading"><Text>)");

    static const std::regex re_sceneheader2("</SceneHeader>");
    output = std::regex_replace(output, re_sceneheader2, "</Text></Paragraph>");

    static const std::regex re_action1("<Action>");
    output = std::regex_replace(output, re_action1,
                                R"(<Paragraph Type="Action"><Text>)");

    static const std::regex re_action2("</Action>");
    output = std::regex_replace(output, re_action2, "</Text></Paragraph>");

    static const std::regex re_character1("<Character>");
    output = std::regex_replace(output, re_character1,
                                R"(<Paragraph Type="Character"><Text>)");

    static const std::regex re_character2("</Character>");
    output = std::regex_replace(output, re_character2, "</Text></Paragraph>");

    static const std::regex re_parenthetical1("<Parenthetical>");
    output = std::regex_replace(output, re_parenthetical1,
                                R"(<Paragraph Type="Parenthetical"><Text>)");

    static const std::regex re_parenthetical2("</Parenthetical>");
    output =
        std::regex_replace(output, re_parenthetical2, "</Text></Paragraph>");

    static const std::regex re_speech1("<Speech>");
    output = std::regex_replace(output, re_speech1,
                                R"(<Paragraph Type="Dialogue"><Text>)");

    static const std::regex re_speech2("</Speech>");
    output = std::regex_replace(output, re_speech2, "</Text></Paragraph>");

    static const std::regex re_dualdialog1("<DualDialog>");
    output =
        std::regex_replace(output, re_dualdialog1, "<Paragraph><DualDialog>");

    static const std::regex re_dualdialog2("</DualDialog>");
    output =
        std::regex_replace(output, re_dualdialog2, "</DualDialog></Paragraph>");

    static const std::regex re_center1("<center>");
    output = std::regex_replace(
        output, re_center1,
        R"(<Paragraph Type="Action" Alignment="Center"><Text>)");

    static const std::regex re_center2("</center>");
    output = std::regex_replace(output, re_center2, "</Text></Paragraph>");

    static const std::regex re_bold1("<b>");
    output = std::regex_replace(output, re_bold1, R"(<Text Style="Bold">)");

    static const std::regex re_bold2("</b>");
    output = std::regex_replace(output, re_bold2, "</Text>");

    static const std::regex re_italic1("<i>");
    output = std::regex_replace(output, re_italic1, R"(<Text Style="Italic">)");

    static const std::regex re_italic2("</i>");
    output = std::regex_replace(output, re_italic2, "</Text>");

    static const std::regex re_underline1("<u>");
    output = std::regex_replace(output, re_underline1,
                                R"(<Text Style="Underline">)");

    static const std::regex re_underline2("</u>");
    output = std::regex_replace(output, re_underline2, "</Text>");

    static const std::regex re_pagebreak1("<PageBreak>");
    output = std::regex_replace(
        output, re_pagebreak1,
        R"(<Paragraph Type="Action" StartsNewPage="Yes"><Text>)");

    static const std::regex re_pagebreak2("</PageBreak>");
    output = std::regex_replace(output, re_pagebreak2, "</Text></Paragraph>");

    static const std::regex re_note1("<Note>");
    output = std::regex_replace(output, re_note1, "<ScriptNote><Text>");

    static const std::regex re_note2("</Note>");
    output = std::regex_replace(output, re_note2, "</Text></ScriptNote>");

    static const std::regex re_dialog("</?Dialog(Left|Right)?>");
    output = std::regex_replace(output, re_dialog, "");

    static const std::regex re_blankline("</?BlankLine>");
    output = std::regex_replace(output, re_blankline, "");

    static const std::regex re_continuation("</?Continuation>");
    output = std::regex_replace(output, re_continuation, "");

    static const std::regex re_newlines(R"(\n+)");
    output = std::regex_replace(output, re_newlines, "\n");

    // Don't know if these work...
    static const std::regex re_lyric1("<Lyric>");
    output = std::regex_replace(output, re_lyric1,
                                R"(<Paragraph Type="Lyric"><Text>)");

    static const std::regex re_lyric2("</Lyric>");
    output = std::regex_replace(output, re_lyric2, "</Text></Paragraph>");
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
    static const std::regex re_blankline("</?BlankLine>");
    output = std::regex_replace(output, re_blankline, "");

    static const std::regex re_continuation("</?Continuation>");
    output = std::regex_replace(output, re_continuation, "");

    static const std::regex re_newlines(R"(\n+)");
    output = std::regex_replace(output, re_newlines, "\n");
  } catch (std::regex_error &e) {
    printf("regex error in ftn2xml\n");
    print_regex_error(e);
  }

  return output;
}
