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

#include <glib-2.0/glib.h>
#include <string.h>

#include "auxiliary.h"
#include "fountain.h"

bool isForced(const std::string &input) {
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

bool isTransition(const std::string &input) {
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

std::string parseTransition(const std::string &input) {
  std::string r = ws_trim(input);
  if (r[0] == '>') {
    r = r.substr(1);
  }

  return to_upper(ws_ltrim(r));
}

bool isSceneHeader(const std::string &input) {
  std::string s = to_upper(ws_ltrim(input));

  if (s.length() < 2) {
    return false;
  }

  if (s[0] == '.' && s[1] != '.') {
    return true;
  }

  if (isForced(s)) {
    return false;
  }

  static GRegex *re_scene_header = nullptr;
  if (!re_scene_header) {
    re_scene_header =
        g_regex_new("^(INT|EXT|EST|INT\\./EXT|INT/EXT|EXT/INT|I/E)[\\.\\ ]",
                    G_REGEX_CASELESS, GRegexMatchFlags(0), nullptr);
  }

  if (g_regex_match(re_scene_header, s.c_str(), GRegexMatchFlags(0), nullptr)) {
    return true;
  }

  return false;
}

std::pair<std::string, std::string> parseSceneHeader(const std::string &input) {
  std::string first;
  std::string second;

  size_t pos = input.find("#");
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

bool isCenter(const std::string &input) {
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

bool isNotation(const std::string &input) {
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

bool isCharacter(const std::string &input) {
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

std::string parseCharacter(const std::string &input) {
  std::string r = ws_ltrim(input);
  if (r[0] == '>') {
    r = ws_ltrim(r.substr(1));
  }
  if (r[r.length() - 1] == '^') {
    r = ws_rtrim(r.substr(0, r.length() - 1));
  }
  return to_upper(r);
}

bool isDualDialog(const std::string &input) {
  size_t len = input.length();

  if (len > 0 && input[len - 1] == '^') {
    return true;
  }

  return false;
}

bool isParenthetical(const std::string &input) {
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

bool isContinuation(const std::string &input) {
  static GRegex *re_whitespace = nullptr;
  if (!re_whitespace) {
    re_whitespace =
        g_regex_new("^\\s+$", G_REGEX_CASELESS, GRegexMatchFlags(0), nullptr);
  }

  if (g_regex_match(re_whitespace, input.c_str(), GRegexMatchFlags(0),
                    nullptr)) {
    return true;
  }

  return false;
}

auto parseKeyValue(const std::string &input) {
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

std::string Script::parseNodeText(const std::string &input) {
  static GRegex *re_underline = nullptr;
  static GRegex *re_bolditalic = nullptr;
  static GRegex *re_bold = nullptr;
  static GRegex *re_italic = nullptr;
  static GRegex *re_note_1 = nullptr;
  static GRegex *re_note_2 = nullptr;
  if (!re_bolditalic) {
    re_bolditalic = g_regex_new("\\*{3}([^*]+?)\\*{3}", G_REGEX_CASELESS,
                                GRegexMatchFlags(0), nullptr);

    re_bold = g_regex_new("\\*{2}([^*]+?)\\*{2}", G_REGEX_CASELESS,
                          GRegexMatchFlags(0), nullptr);

    re_italic = g_regex_new("\\*{1}([^*]+?)\\*{1}", G_REGEX_CASELESS,
                            GRegexMatchFlags(0), nullptr);

    re_underline = g_regex_new("\\_{1}([^_]+)\\_{1}", G_REGEX_CASELESS,
                               GRegexMatchFlags(0), nullptr);

    re_note_1 = g_regex_new("(?s)\\[{2}(.*?)\\]{2}", G_REGEX_CASELESS,
                            GRegexMatchFlags(0), nullptr);

    re_note_2 = g_regex_new("(?s)\\[{2}(.*?)$", G_REGEX_CASELESS,
                            GRegexMatchFlags(0), nullptr);
  }
  gchar *tmp_str_1 = nullptr;
  gchar *tmp_str_2 = nullptr;

  if (input.find("*") != std::string::npos) {
    tmp_str_1 =
        g_regex_replace(re_bolditalic, input.c_str(), -1, 0,
                        "<b><i>\\1</i></b>", GRegexMatchFlags(0), nullptr);
    tmp_str_2 = g_regex_replace(re_bold, tmp_str_1, -1, 0, "<b>\\1</b>",
                                GRegexMatchFlags(0), nullptr);
    g_free(tmp_str_1);
    tmp_str_1 = g_regex_replace(re_italic, tmp_str_2, -1, 0, "<i>\\1</i>",
                                GRegexMatchFlags(0), nullptr);
    g_free(tmp_str_2);
  } else {
    tmp_str_1 = g_strdup(input.c_str());
  }

  if (input.find("[[") != std::string::npos) {
    tmp_str_2 = g_regex_replace(re_note_1, tmp_str_1, -1, 0, "<note>\\1</note>",
                                GRegexMatchFlags(0), nullptr);
    g_free(tmp_str_1);
    tmp_str_1 = g_regex_replace(re_note_2, tmp_str_2, -1, 0, "<note>\\1</note>",
                                GRegexMatchFlags(0), nullptr);
    g_free(tmp_str_2);
  }

  if (input.find("_") != std::string::npos) {
    tmp_str_2 = g_regex_replace(re_underline, tmp_str_1, -1, 0, "<u>\\1</u>",
                                GRegexMatchFlags(0), nullptr);
    g_free(tmp_str_1);
    tmp_str_1 = tmp_str_2;
  }

  std::string output(tmp_str_1);
  g_free(tmp_str_1);

  return output;
}

std::string ScriptNode::to_string() const {
  static int dialog_state = 0;
  std::string output;

  switch (type) {
    case ScriptNodeType::ftnKeyValue:
      output += "<meta>\n<key>" + key + "</key>\n<value>" + value +
                "</value>\n</meta>";
      break;
    case ScriptNodeType::ftnPageBreak:
      if (dialog_state) {
        dialog_state == 1   ? output += "</Dialog>\n"
        : dialog_state == 2 ? output += "</DialogLeft>\n"
                            : output += "</DialogRight>\n</DualDialog>\n";
        dialog_state = 0;
      }
      output += "<PageBreak></PageBreak>";
      break;
    case ScriptNodeType::ftnBlankLine:
      if (dialog_state) {
        dialog_state == 1   ? output += "</Dialog>\n"
        : dialog_state == 2 ? output += "</DialogLeft>\n"
                            : output += "</DialogRight>\n</DualDialog>\n";
        dialog_state = 0;
      }
      output += "<BlankLine></BlankLine>";
      break;
    case ScriptNodeType::ftnContinuation:
      output += "<Continuation>" + value + "</Continuation>";
      break;
    case ScriptNodeType::ftnSceneHeader:
      if (!key.empty()) {
        output += "<SceneHeader><SceneNumL>" + key + "</SceneNumL>" + value +
                  +"<SceneNumR>" + key + "</SceneNumR></SceneHeader>";
      } else {
        output += "<SceneHeader>" + value + "</SceneHeader>";
      }
      break;
    case ScriptNodeType::ftnAction:
      output += "<Action>" + value + "</Action>";
      break;
    case ScriptNodeType::ftnTransition:
      output += "<Transition>" + value + "</Transition>";
      break;
    case ScriptNodeType::ftnDialog:
      dialog_state = 1;
      output += "<Dialog>" + key;
      break;
    case ScriptNodeType::ftnDialogLeft:
      dialog_state = 2;
      output += "<DualDialog>\n<DialogLeft>" + value;
      break;
    case ScriptNodeType::ftnDialogRight:
      dialog_state = 3;
      output += "<DialogRight>" + value;
      break;
    case ScriptNodeType::ftnCharacter:
      output += "<Character>" + value + "</Character>";
      break;
    case ScriptNodeType::ftnParenthetical:
      output += "<Parenthetical>" + value + "</Parenthetical>";
      break;
    case ScriptNodeType::ftnSpeech:
      output += "<Speech>" + value + "</Speech>";
      break;
    case ScriptNodeType::ftnNotation:
      output += "<Note>" + value + "</Note>";
      break;
    case ScriptNodeType::ftnLyric:
      output += "<Lyric>" + value + "</Lyric>";
      break;
    case ScriptNodeType::ftnSection:
      output += "<SectionH" + key + ">" + value + "</SectionH" + key + ">";
      break;
    case ScriptNodeType::ftnSynopsis:
      output += "<SynopsisH" + key + ">" + value + "</SynopsisH" + key + ">";
      break;
    case ScriptNodeType::ftnUnknown:
    default:
      output += "<Unknown>" + value + "</Unknown>";
      break;
  }
  return output;
}

void Script::parseFountain(const std::string &text) {
  if (!text.length()) {
    return;
  }

  std::vector<std::string> lines;

  // remove comments
  {
    static GRegex *re_comment = nullptr;
    if (!re_comment) {
      re_comment = g_regex_new("(?s)/\\*.*?\\*/", G_REGEX_MULTILINE,
                               GRegexMatchFlags(0), nullptr);
    }
    char *cs = g_regex_replace_literal(re_comment, text.c_str(), -1, 0, "",
                                       GRegexMatchFlags(0), nullptr);
    std::string s = std::string(cs);
    g_free(cs);
    lines = split_lines(s);
  }

  // determine whether to extract header
  static GRegex *re_has_header = nullptr;
  if (!re_has_header) {
    re_has_header = g_regex_new("^[^\\s:]+:\\s.*$", G_REGEX_MULTILINE,
                                GRegexMatchFlags(0), nullptr);
  }
  bool has_header = false;
  if (g_regex_match(re_has_header, text.c_str(), GRegexMatchFlags(0),
                    nullptr)) {
    has_header = true;
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
        append_text(kv.second);
        continue;
      }
      if (line.length() == 0) {
        has_header = false;
      } else {
        append_text(ws_rtrim(s));
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
        append_text(line);
      } else {
        new_node(ScriptNodeType::ftnContinuation, line);
        end_node();
      }
      continue;
    }

    if (isNotation(s)) {
      if (line[0] == ' ' || line[0] == '\t' || line[line.length() - 1] == ' ' ||
          line[line.length() - 1] == '\t') {
        append_text(s);
      } else {
        append_text(s);
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
      append_text(parseTransition(s));
      end_node();
      continue;
    }

    // Scene Header
    if (curr_node.type == ScriptNodeType::ftnUnknown && isSceneHeader(s)) {
      auto scene = parseSceneHeader(s);

      new_node(ScriptNodeType::ftnSceneHeader, scene.second);
      append_text(scene.first);
      end_node();
      continue;
    }

    // Character
    if (curr_node.type == ScriptNodeType::ftnUnknown && isCharacter(s)) {
      if (isDualDialog(s)) {
        // modify previous dialog node
        bool prev_dialog_modded = false;
        for (size_t pos = nodes.size() - 1; pos >= 0; pos--) {
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
        append_text(parseCharacter(s));
        end_node();
      } else {
        new_node(ScriptNodeType::ftnDialog, "");
        new_node(ScriptNodeType::ftnCharacter, "");
        append_text(parseCharacter(s));
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
          append_text(ws_trim(s));
          end_node();
          continue;
        }
      }
    }

    // Speech
    if (curr_node.type == ScriptNodeType::ftnSpeech) {
      append_text(s);
      continue;
    } else if (!nodes.empty()) {
      ScriptNodeType ct = nodes.back().type;
      if (ct == ScriptNodeType::ftnParenthetical ||
          ct == ScriptNodeType::ftnCharacter) {
        new_node(ScriptNodeType::ftnSpeech, "");
        append_text(s);
        continue;
      }
    }

    // Section, can only be forced
    if (s.length() > 0 && s[0] == '#') {
      for (int i = 1; i < 6; i++) {
        if (s.length() > i && s[i] == '#') {
          if (i == 5) {
            new_node(ScriptNodeType::ftnSection, std::to_string(i + 1));
            append_text(s.substr(i + 1));
            currSection = i + 1;
            break;
          }
        } else {
          new_node(ScriptNodeType::ftnSection, std::to_string(i));
          append_text(s.substr(i));
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
      append_text(ws_ltrim(s.substr(1)));
      end_node();
      continue;
    }

    // Lyric, can only be forced
    if (s.length() > 1 && s[0] == '~') {
      new_node(ScriptNodeType::ftnLyric, "");
      append_text(s.substr(1));
      end_node();
      continue;
    }

    // Probably Action
    // Note: Leading whitespace is retained in Action
    if (curr_node.type == ScriptNodeType::ftnAction) {
      if (isCenter(s)) {
        append_text("<center>" + ws_trim(s.substr(1, s.length() - 2)) +
                    "</center>");
      } else {
        append_text(line);
      }
      continue;
    }
    if (isCenter(s)) {
      new_node(ScriptNodeType::ftnAction, "");
      append_text("<center>" + ws_trim(s.substr(1, s.length() - 2)) +
                  "</center>");
      continue;
    }
    if (s.length() > 1 && s[0] == '!') {
      new_node(ScriptNodeType::ftnAction, "");
      append_text(s.substr(1));
      continue;
    }
    if (curr_node.type == ScriptNodeType::ftnUnknown) {
      new_node(ScriptNodeType::ftnAction, "");
      append_text(line);
      continue;
    }
  }

  end_node();
}

char *ftn2str(const char *input) {
  std::string output("<Fountain>\n");

  std::string s(input);
  Script script;
  script.parseFountain(s);

  for (auto node : script.nodes) {
    output += node.to_string() + "\n";
  }

  output += "\n</Fountain>";
  return strdup(output.c_str());
}

// semi-compatible with screenplain
char *ftn2html(const char *input, const char *css_fn) {
  std::string output(
      "<!DOCTYPE html>\n"
      "<html>\n<head>\n");

  if (css_fn) {
    output += "<link rel=\"stylesheet\" type=\"text/css\" href=\"file://";
    output += css_fn;
    output += "\">\n";
  }

  output +=
      "</head>\n<body>\n"
      "<div id=\"wrapper\" class=\"fountain\">\n";

  std::string s(input);
  Script script;
  script.parseFountain(s);

  for (auto node : script.nodes) {
    if (node.type != ScriptNodeType::ftnContinuation &&
        node.type != ScriptNodeType::ftnKeyValue &&
        node.type != ScriptNodeType::ftnUnknown) {
      output += node.to_string() + "\n";
    }
  }

  output += "\n</div>\n</body>\n</html>\n";

  GString *gstr_output = g_string_new(output.c_str());

  // Screenplay elements
  g_string_replace(gstr_output, "<Transition>", "<div class=\"transition\">",
                   0);
  g_string_replace(gstr_output, "</Transition>", "</div>", 0);

  g_string_replace(gstr_output, "<SceneHeader>", "<div class=\"sceneheader\">",
                   0);
  g_string_replace(gstr_output, "</SceneHeader>", "</div>", 0);
  g_string_replace(gstr_output, "<Action>", "<div class=\"action\">", 0);
  g_string_replace(gstr_output, "</Action>", "</div>", 0);
  g_string_replace(gstr_output, "<Lyric>", "<div class=\"lyric\">", 0);
  g_string_replace(gstr_output, "</Lyric>", "</div>", 0);

  // Dialog
  g_string_replace(gstr_output, "<Character>", "<p class=\"character\">", 0);
  g_string_replace(gstr_output, "</Character>", "</p>", 0);
  g_string_replace(gstr_output, "<Parenthetical>",
                   "<p class=\"parenthetical\">", 0);
  g_string_replace(gstr_output, "</Parenthetical>", "</p>", 0);
  g_string_replace(gstr_output, "<Speech>", "<p class=\"speech\">", 0);
  g_string_replace(gstr_output, "</Speech>", "</p>", 0);
  g_string_replace(gstr_output, "<Dialog>", "<div class=\"dialog\">", 0);
  g_string_replace(gstr_output, "</Dialog>", "</div>", 0);
  g_string_replace(gstr_output, "<DualDialog>", "<div class=\"dual\">", 0);
  g_string_replace(gstr_output, "</DualDialog>", "</div>", 0);
  g_string_replace(gstr_output, "<DialogLeft>", "<div class=\"left\">", 0);
  g_string_replace(gstr_output, "</DialogLeft>", "</div>", 0);
  g_string_replace(gstr_output, "<DialogRight>", "<div class=\"right\">", 0);
  g_string_replace(gstr_output, "</DialogRight>", "</div>", 0);

  g_string_replace(gstr_output, "<PageBreak>", "<div class=\"page-break\">", 0);
  g_string_replace(gstr_output, "</PageBreak>", "</div>", 0);

  // Notes
  g_string_replace(gstr_output, "<Note>", "<div class=\"note\">", 0);
  g_string_replace(gstr_output, "</Note>", "</div>", 0);

  // Discard
  g_string_replace(gstr_output, "<BlankLine>", "", 0);
  g_string_replace(gstr_output, "</BlankLine>", "", 0);
  g_string_replace(gstr_output, "<Continuation>", "", 0);
  g_string_replace(gstr_output, "</Continuation>", "", 0);

  // Cleanup
  g_string_replace(gstr_output, "\n\n", "\n", 0);
  g_string_replace(gstr_output, "\n\n", "\n", 0);

  return g_string_free(gstr_output, false);
}

char *ftn2fdx(const char *input) {
  std::string output(
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n"
      "<FinalDraft DocumentType=\"Script\" Template=\"No\" Version=\"1\">\n"
      "<Content>\n");
  std::string s(input);
  Script script;
  script.parseFountain(s);

  for (auto node : script.nodes) {
    if (node.type != ScriptNodeType::ftnContinuation &&
        node.type != ScriptNodeType::ftnKeyValue &&
        node.type != ScriptNodeType::ftnSection &&
        node.type != ScriptNodeType::ftnSynopsis &&
        node.type != ScriptNodeType::ftnUnknown) {
      output += node.to_string() + "\n";
    }
  }

  output += "\n</Content>\n</FinalDraft>\n";

  GString *gstr_output = g_string_new(output.c_str());

  // Screenplay elements
  g_string_replace(gstr_output, "<Transition>",
                   "<Paragraph Type=\"Transition\"><Text>", 0);
  g_string_replace(gstr_output, "</Transition>", "</Text></Paragraph>", 0);
  g_string_replace(gstr_output, "<SceneHeader>",
                   "<Paragraph Type=\"Scene Heading\"><Text>", 0);
  g_string_replace(gstr_output, "</SceneHeader>", "</Text></Paragraph>", 0);
  g_string_replace(gstr_output, "<Action>", "<Paragraph Type=\"Action\"><Text>",
                   0);
  g_string_replace(gstr_output, "</Action>", "</Text></Paragraph>", 0);

  // Dialog
  g_string_replace(gstr_output, "<Character>",
                   "<Paragraph Type=\"Character\"><Text>", 0);
  g_string_replace(gstr_output, "</Character>", "</Text></Paragraph>", 0);
  g_string_replace(gstr_output, "<Parenthetical>",
                   "<Paragraph Type=\"Parenthetical\"><Text>", 0);
  g_string_replace(gstr_output, "</Parenthetical>", "</Text></Paragraph>", 0);
  g_string_replace(gstr_output, "<Speech>",
                   "<Paragraph Type=\"Dialogue\"><Text>", 0);
  g_string_replace(gstr_output, "</Speech>", "</Text></Paragraph>", 0);
  g_string_replace(gstr_output, "<DualDialog>", "<Paragraph><DualDialog>", 0);
  g_string_replace(gstr_output, "</DualDialog>", "</DualDialog></Paragraph>",
                   0);

  // Formating
  g_string_replace(gstr_output, "<center>",
                   "<Paragraph Type=\"Action\" Alignment=\"Center\"><Text>", 0);
  g_string_replace(gstr_output, "</center>", "</Text></Paragraph>", 0);
  g_string_replace(gstr_output, "<b>", "<Text Style=\"Bold\">", 0);
  g_string_replace(gstr_output, "</b>", "</Text>", 0);
  g_string_replace(gstr_output, "<u>", "<Text Style=\"Underline\">", 0);
  g_string_replace(gstr_output, "</u>", "</Text>", 0);
  g_string_replace(gstr_output, "<i>", "<Text Style=\"Italic\">", 0);
  g_string_replace(gstr_output, "</i>", "</Text>", 0);

  g_string_replace(gstr_output, "<PageBreak>",
                   "<Paragraph Type=\"Action\" StartsNewPage=\"Yes\"><Text>",
                   0);
  g_string_replace(gstr_output, "</PageBreak>", "</Text></Paragraph>", 0);

  // Notes
  g_string_replace(gstr_output, "<Note>", "<ScriptNote><Text>", 0);
  g_string_replace(gstr_output, "</Note>", "</Text></ScriptNote>", 0);

  // Discard
  g_string_replace(gstr_output, "<Dialog>", "", 0);
  g_string_replace(gstr_output, "</Dialog>", "", 0);
  g_string_replace(gstr_output, "<DialogLeft>", "", 0);
  g_string_replace(gstr_output, "</DialogLeft>", "", 0);
  g_string_replace(gstr_output, "<DialogRight>", "", 0);
  g_string_replace(gstr_output, "</DialogRight>", "", 0);
  g_string_replace(gstr_output, "<BlankLine>", "", 0);
  g_string_replace(gstr_output, "</BlankLine>", "", 0);
  g_string_replace(gstr_output, "<Continuation>", "", 0);
  g_string_replace(gstr_output, "</Continuation>", "", 0);

  // Cleanup
  g_string_replace(gstr_output, "\n\n", "\n", 0);
  g_string_replace(gstr_output, "\n\n", "\n", 0);

  // Don't know if these work...
  g_string_replace(gstr_output, "<Lyric>", "<Paragraph Type=\"Lyric\"><Text>",
                   0);
  g_string_replace(gstr_output, "</Lyric>", "</Text></Paragraph>", 0);

  return g_string_free(gstr_output, false);
}

char *ftn2html2(const char *input, const char *css_fn) {
  std::string output(
      "<!DOCTYPE html>\n"
      "<html>\n<head>\n");

  if (css_fn) {
    output += "<link rel=\"stylesheet\" type=\"text/css\" href=\"file://";
    output += css_fn;
    output += "\">\n";
  }

  output +=
      "</head>\n<body>\n"
      "<Fountain>\n";

  std::string s(input);
  Script script;
  script.parseFountain(s);

  for (auto node : script.nodes) {
    if (node.type != ScriptNodeType::ftnContinuation &&
        node.type != ScriptNodeType::ftnKeyValue &&
        node.type != ScriptNodeType::ftnUnknown) {
      output += node.to_string() + "\n";
    }
  }

  output += "\n</Fountain>\n</body>\n</html>\n";

  GString *gstr_output = g_string_new(output.c_str());

  // Discard
  g_string_replace(gstr_output, "<BlankLine>", "", 0);
  g_string_replace(gstr_output, "</BlankLine>", "", 0);
  g_string_replace(gstr_output, "<Continuation>", "", 0);
  g_string_replace(gstr_output, "</Continuation>", "", 0);

  // Cleanup
  g_string_replace(gstr_output, "\n\n", "\n", 0);
  g_string_replace(gstr_output, "\n\n", "\n", 0);

  return g_string_free(gstr_output, false);
}
