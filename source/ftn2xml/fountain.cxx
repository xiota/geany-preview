/*
 * SPDX-FileCopyrightText: Copyright 2021-2024 xiota
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "fountain.hxx"

#include <cstring>

#include "auxiliary.hxx"

namespace Fountain {

namespace {

bool isForced(const std::string & input) {
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

bool isTransition(const std::string & input) {
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
  const char * transitions[] = {
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

std::string parseTransition(const std::string & input) {
  if (input.empty()) {
    return {};
  }
  if (input[0] == '>') {
    return to_upper(ws_trim(input.substr(1)));
  }
  return to_upper(ws_trim(input));
}

bool isSceneHeader(const std::string & input) {
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
        R"(^(INT|EXT|EST|INT\.?/EXT|EXT\.?/INT|I/E|E/I)[\.\ ])", std::regex_constants::icase
    );

    if (std::regex_search(input, re_scene_header)) {
      return true;
    }
  } catch (std::regex_error & e) {
    print_regex_error(e, __FILE__, __LINE__);
  }
  return false;
}

// returns <scene description, scene number>
// Note: in should already be length checked by isSceneHeader()
auto parseSceneHeader(const std::string & input) {
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

bool isCenter(const std::string & input) {
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

bool isNotation(const std::string & input) {
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

bool isCharacter(const std::string & input) {
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
std::string parseCharacter(const std::string & input) {
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
bool isDualDialog(const std::string & input) {
  if (input.back() == '^') {
    return true;
  }

  return false;
}

bool isParenthetical(const std::string & input) {
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

bool isContinuation(const std::string & input) {
  if (input.empty()) {
    return false;
  }

  if (input.find_first_not_of(FOUNTAIN_WHITESPACE) == std::string::npos) {
    return true;
  }
  return false;
}

auto parseKeyValue(const std::string & input) {
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
std::string & parseEscapeSequences_inplace(std::string & input) {
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

std::string ScriptNode::to_string(const int & flags) const {
  static int dialog_state = 0;
  std::string output;

  switch (type) {
    case ScriptNodeType::ftnKeyValue:
      if (flags & type) {
        break;
      }
      output = "<meta>\n<key>" + key + "</key>\n<value>" + value + "</value>\n</meta>\n";
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
        output = "<SceneHeader><SceneNumL>" + key + "</SceneNumL>" + value + +"<SceneNumR>" +
                 key + "</SceneNumR></SceneHeader>\n";
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

std::string Script::to_string(const int & flags) const {
  std::string output{"<Fountain>\n"};

  for (auto node : nodes) {
    output += node.to_string(flags);
  }

  output += "\n</Fountain>\n";
  return output;
}

std::string Script::parseNodeText(const std::string & input) {
  try {
    static const std::regex re_bolditalic(R"(\*{3}([^*]+?)\*{3})");
    static const std::regex re_bold(R"(\*{2}([^*]+?)\*{2})");
    static const std::regex re_italic(R"(\*{1}([^*]+?)\*{1})");
    static const std::regex re_underline(R"(_([^_\n]+)_)");
    std::string output = std::regex_replace(input, re_bolditalic, "<b><i>$1</i></b>");
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
  } catch (std::regex_error & e) {
    print_regex_error(e, __FILE__, __LINE__);
    return input;
  }
}

void Script::parseFountain(const std::string & text) {
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
  } catch (std::regex_error & e) {
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
  } catch (std::regex_error & e) {
    print_regex_error(e, __FILE__, __LINE__);
  }

  // used for synposis
  int currSection = 1;

  // process line-by-line
  for (auto & line : lines) {
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
        if (ct == ScriptNodeType::ftnParenthetical || ct == ScriptNodeType::ftnCharacter ||
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
      if (ct == ScriptNodeType::ftnParenthetical || ct == ScriptNodeType::ftnCharacter) {
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
std::string ftn2screenplain(
    const std::string & input, const std::string & css_fn, const bool & embed_css
) {
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

  output += script.to_string(
      Fountain::ScriptNodeType::ftnContinuation | Fountain::ScriptNodeType::ftnKeyValue |
      Fountain::ScriptNodeType::ftnUnknown
  );

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

    replace_all_inplace(output, "<Parenthetical>", R"(<p class="parenthetical">)");
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
  } catch (std::regex_error & e) {
    print_regex_error(e, __FILE__, __LINE__);
  }

  return output;
}

// html similar to textplay html output (can use the same css files)
std::string ftn2textplay(
    const std::string & input, const std::string & css_fn, const bool & embed_css
) {
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

  output += script.to_string(
      Fountain::ScriptNodeType::ftnContinuation | Fountain::ScriptNodeType::ftnKeyValue |
      Fountain::ScriptNodeType::ftnUnknown
  );

  output += "\n</div>\n</body>\n</html>\n";

  try {
    replace_all_inplace(output, "<Transition>", R"(<h3 class="right-transition">)");
    replace_all_inplace(output, "</Transition>", "</h3>");

    replace_all_inplace(output, "<SceneHeader>", R"(<h2 class="full-slugline">)");
    replace_all_inplace(output, "</SceneHeader>", "</h2>");

    replace_all_inplace(output, "<Action>", R"(<p class="action">)");
    replace_all_inplace(output, "</Action>", "</p>");

    replace_all_inplace(output, "<Lyric>", R"(<span class="lyric">)");
    replace_all_inplace(output, "</Lyric>", "</span>");

    replace_all_inplace(output, "<Character>", R"(<dt class="character">)");
    replace_all_inplace(output, "</Character>", "</dt>");

    replace_all_inplace(output, "<Parenthetical>", R"(<dd class="parenthetical">)");
    replace_all_inplace(output, "</Parenthetical>", "</dd>");

    replace_all_inplace(output, "<Speech>", R"(<dd class="dialogue">)");
    replace_all_inplace(output, "</Speech>", "</dd>");

    replace_all_inplace(output, "<Dialog>", R"(<div class="dialog">)");
    replace_all_inplace(output, "</Dialog>", "</div>");

    replace_all_inplace(output, "<DialogDual>", R"(<div class="dialog_wrapper">)");
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

  } catch (std::regex_error & e) {
    print_regex_error(e, __FILE__, __LINE__);
  }

  return output;
}

std::string ftn2fdx(const std::string & input) {
  std::string output{R"(<?xml version="1.0" encoding="UTF-8" standalone="no" ?>)"};
  output += '\n';
  output += R"(<FinalDraft DocumentType="Script" Template="No" Version="1">)";
  output += "\n<Content>\n";

  Fountain::Script script;
  script.parseFountain(input);

  output += script.to_string(
      Fountain::ScriptNodeType::ftnContinuation | Fountain::ScriptNodeType::ftnKeyValue |
      Fountain::ScriptNodeType::ftnSection | Fountain::ScriptNodeType::ftnSynopsis |
      Fountain::ScriptNodeType::ftnUnknown
  );

  output += "\n</Content>\n</FinalDraft>\n";

  try {
    replace_all_inplace(output, "<Transition>", R"(<Paragraph Type="Transition"><Text>)");
    replace_all_inplace(output, "</Transition>", "</Text></Paragraph>");

    replace_all_inplace(output, "<SceneHeader>", R"(<Paragraph Type="Scene Heading"><Text>)");
    replace_all_inplace(output, "</SceneHeader>", "</Text></Paragraph>");

    replace_all_inplace(output, "<Action>", R"(<Paragraph Type="Action"><Text>)");
    replace_all_inplace(output, "</Action>", "</Text></Paragraph>");

    replace_all_inplace(output, "<Character>", R"(<Paragraph Type="Character"><Text>)");
    replace_all_inplace(output, "</Character>", "</Text></Paragraph>");

    replace_all_inplace(output, "<Parenthetical>", R"(<Paragraph Type="Parenthetical"><Text>)");
    replace_all_inplace(output, "</Parenthetical>", "</Text></Paragraph>");

    replace_all_inplace(output, "<Speech>", R"(<Paragraph Type="Dialogue"><Text>)");
    replace_all_inplace(output, "</Speech>", "</Text></Paragraph>");

    replace_all_inplace(output, "<DualDialog>", "<Paragraph><DualDialog>");
    replace_all_inplace(output, "</DualDialog>", "</DualDialog></Paragraph>");

    replace_all_inplace(
        output, "<ActionCenter>", R"(<Paragraph Type="Action" Alignment="Center"><Text>)"
    );
    replace_all_inplace(output, "</ActionCenter>", "</Text></Paragraph>");

    replace_all_inplace(output, "<b>", R"(<Text Style="Bold">)");
    replace_all_inplace(output, "</b>", "</Text>");

    replace_all_inplace(output, "<i>", R"(<Text Style="Italic">)");
    replace_all_inplace(output, "</i>", "</Text>");

    replace_all_inplace(output, "<u>", R"(<Text Style="Underline">)");
    replace_all_inplace(output, "</u>", "</Text>");

    replace_all_inplace(
        output, "<PageBreak>", R"(<Paragraph Type="Action" StartsNewPage="Yes"><Text>)"
    );
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
  } catch (std::regex_error & e) {
    print_regex_error(e, __FILE__, __LINE__);
  }

  return output;
}

std::string ftn2xml(
    const std::string & input, const std::string & css_fn, const bool & embed_css
) {
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

  output += script.to_string(
      Fountain::ScriptNodeType::ftnContinuation | Fountain::ScriptNodeType::ftnKeyValue |
      Fountain::ScriptNodeType::ftnUnknown
  );

  output += "\n</body>\n</html>\n";

  try {
    static const std::regex re_newlines(R"(\n+)");
    output = std::regex_replace(output, re_newlines, "\n");
  } catch (std::regex_error & e) {
    print_regex_error(e, __FILE__, __LINE__);
  }

  return output;
}

std::string ftn2html(
    const std::string & input, const std::string & css_fn, const bool & embed_css
) {
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

  output += script.to_string(
      Fountain::ScriptNodeType::ftnContinuation | Fountain::ScriptNodeType::ftnKeyValue |
      Fountain::ScriptNodeType::ftnUnknown
  );

  output += "\n</div>\n</body>\n</html>\n";

  try {
    replace_all_inplace(output, "<Fountain>", R"(<div class="Fountain">)");
    replace_all_inplace(output, "</Fountain>", "</div>");

    replace_all_inplace(output, "<Transition>", R"(<div class="Transition">)");
    replace_all_inplace(output, "</Transition>", "</div>");

    replace_all_inplace(output, "<SceneHeader>", R"(<div class="SceneHeader">)");
    replace_all_inplace(output, "</SceneHeader>", "</div>");

    replace_all_inplace(output, "<Action>", R"(<div class="Action">)");
    replace_all_inplace(output, "</Action>", "</div>");

    replace_all_inplace(output, "<Lyric>", R"(<div class="Lyric">)");
    replace_all_inplace(output, "</Lyric>", "</div>");

    replace_all_inplace(output, "<Character>", R"(<div class="Character">)");
    replace_all_inplace(output, "</Character>", "</div>");

    replace_all_inplace(output, "<Parenthetical>", R"(<div class="Parenthetical">)");
    replace_all_inplace(output, "</Parenthetical>", "</div>");

    replace_all_inplace(output, "<Speech>", R"(<div class="Speech">)");
    replace_all_inplace(output, "</Speech>", "</div>");

    replace_all_inplace(output, "<Dialog>", R"(<div class="Dialog">)");
    replace_all_inplace(output, "</Dialog>", "</div>");

    replace_all_inplace(output, "<DialogDual>", R"(<div class="DialogDual">)");
    replace_all_inplace(output, "</DialogDual>", "</div>");

    replace_all_inplace(output, "<DialogLeft>", R"(<div class="DialogLeft">)");
    replace_all_inplace(output, "</DialogLeft>", "</div>");

    replace_all_inplace(output, "<DialogRight>", R"(<div class="DialogRight">)");
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
      replace_all_inplace(
          output, "<SectionH" + lvl + ">", R"(<div class="SectionH)" + lvl + R"(">)"
      );
      replace_all_inplace(output, "</SectionH" + lvl + ">", "</div>");

      replace_all_inplace(
          output, "<SynopsisH" + lvl + ">", R"(<div class="SynopsisH)" + lvl + R"(">)"
      );
      replace_all_inplace(output, "</SynopsisH" + lvl + ">", "</div>");
    }

    static const std::regex re_newlines(R"(\n+)");
    output = std::regex_replace(output, re_newlines, "\n");
  } catch (std::regex_error & e) {
    print_regex_error(e, __FILE__, __LINE__);
  }

  return output;
}

}  // namespace Fountain

#if ENABLE_EXPORT_PDF

#include <podofo/podofo.h>

#if (PODOFO_VERSION_MINOR < 10) && (PODOFO_VERSION_MAJOR < 1)
#include "pdf-export-0.9.hxx"
#else
#include "pdf-export-0.10.hxx"
#endif

#endif  // ENABLE_EXPORT_PDF
