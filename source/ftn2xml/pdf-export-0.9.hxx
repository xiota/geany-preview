/*
 * SPDX-FileCopyrightText: Copyright 2021-2024 xiota
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

namespace Fountain {

namespace {  // PDF export - using PoDoFo 0.9.x

void add_char(
    const std::string & strAdd, std::string & strNormal, std::string & strBold,
    std::string & strItalic, std::string & strBoldItalic, std::string & strUnderline,
    const bool & bBold, const bool & bItalic, const bool & bUnderline
) {
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

auto split_formatting(const std::string & input) {
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
    return std::make_tuple(strNormal, strBold, strItalic, strBoldItalic, strUnderline);
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
                add_char(
                    input.substr(pos, 2), strNormal, strBold, strItalic, strBoldItalic,
                    strUnderline, bBold, bItalic, bUnderline
                );
                ++pos;
              }
              break;
            case 'i':
              if (input[pos + 2] == '>') {
                bItalic = true;
                pos += 2;
              } else {
                add_char(
                    input.substr(pos, 2), strNormal, strBold, strItalic, strBoldItalic,
                    strUnderline, bBold, bItalic, bUnderline
                );
                ++pos;
              }
              break;
            case 'u':
              if (input[pos + 2] == '>') {
                bUnderline = true;
                pos += 2;
              } else {
                add_char(
                    input.substr(pos, 2), strNormal, strBold, strItalic, strBoldItalic,
                    strUnderline, bBold, bItalic, bUnderline
                );
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
                      add_char(
                          input.substr(pos, 3), strNormal, strBold, strItalic, strBoldItalic,
                          strUnderline, bBold, bItalic, bUnderline
                      );
                      pos += 2;
                    }
                    break;
                  case 'i':
                    if (input[pos + 3] == '>') {
                      bItalic = false;
                      pos += 3;
                    } else {
                      add_char(
                          input.substr(pos, 3), strNormal, strBold, strItalic, strBoldItalic,
                          strUnderline, bBold, bItalic, bUnderline
                      );
                      pos += 2;
                    }
                    break;
                  case 'u':
                    if (input[pos + 3] == '>') {
                      bUnderline = false;
                      pos += 3;
                    } else {
                      add_char(
                          input.substr(pos, 3), strNormal, strBold, strItalic, strBoldItalic,
                          strUnderline, bBold, bItalic, bUnderline
                      );
                      pos += 2;
                    }
                    break;
                  default:
                    add_char(
                        input.substr(pos, 3), strNormal, strBold, strItalic, strBoldItalic,
                        strUnderline, bBold, bItalic, bUnderline
                    );
                    pos += 2;
                    break;
                }
              }
              break;
            default:
              add_char(
                  input.substr(pos, 2), strNormal, strBold, strItalic, strBoldItalic,
                  strUnderline, bBold, bItalic, bUnderline
              );
              ++pos;
              break;
          }
        } else {
          add_char(
              std::string{input[pos]}, strNormal, strBold, strItalic, strBoldItalic,
              strUnderline, bBold, bItalic, bUnderline
          );
        }
        break;
      default:
        add_char(
            std::string{input[pos]}, strNormal, strBold, strItalic, strBoldItalic, strUnderline,
            bBold, bItalic, bUnderline
        );
        break;
    }
  }

  return std::make_tuple(strNormal, strBold, strItalic, strBoldItalic, strUnderline);
}

std::string & center_text_inplace(std::string & text, const int line_length = 60) {
  text = std::string(int((line_length - text.length() + 1) / 2.), ' ') + text;
  return text;
}

std::vector<std::string> wrap_text(const std::string & text, const int width) {
  std::vector<std::string> words = split_string(ws_rtrim(text), " ");
  std::vector<std::string> lines;
  const std::size_t cwidth = (width * 10) / 72;

  std::string ln;
  ln.reserve(cwidth + 100);
  std::size_t len = 0;

  for (auto & w : words) {
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

std::vector<std::string> pdfTextLines(const std::string & text, const int width = 432) {
  std::vector<std::string> temp = split_string(ws_rtrim(text), "\n");
  std::vector<std::string> lines;

  for (auto & i : temp) {
    for (auto & j : wrap_text(i, width)) {
      lines.push_back(j);
    }
  }

  return lines;
}

void pdfTextAdd(
    PoDoFo::PdfStreamedDocument & document, PoDoFo::PdfPainter & painter,
    const std::string & text, const std::string & font, int line, const int width = 432,
    const int left_margin = 108, const int bottom_margin = 72, const int print_height = 648,
    PoDoFo::EPdfAlignment eAlignment = PoDoFo::ePdfAlignment_Left
) {
  if (font == "title") {
    PoDoFo::PdfFont * pFontCustom = document.CreateFont("Alegreya SC Medium");
    pFontCustom->SetFontSize(24.0);
    painter.SetFont(pFontCustom);
    painter.DrawMultiLineText(
        left_margin, bottom_margin, width, print_height - 12 * line,
        PoDoFo::PdfString((const PoDoFo::pdf_utf8 *)text.c_str()), eAlignment,
        PoDoFo::ePdfVerticalAlignment_Top
    );
    return;
  }

  PoDoFo::PdfFont * pFontNormal = nullptr;
  if (font == "bold" || font == "sceneheader") {
    pFontNormal = document.CreateFont(PODOFO_HPDF_FONT_COURIER_BOLD);
  } else if (font == "italic" || font == "lyric") {
    pFontNormal = document.CreateFont(PODOFO_HPDF_FONT_COURIER_OBLIQUE);
  } else {
    pFontNormal = document.CreateFont(PODOFO_HPDF_FONT_COURIER);
  }

  PoDoFo::PdfFont * pFontItalic = document.CreateFont(PODOFO_HPDF_FONT_COURIER_OBLIQUE);

  PoDoFo::PdfFont * pFontBold = document.CreateFont(PODOFO_HPDF_FONT_COURIER_BOLD);

  PoDoFo::PdfFont * pFontBoldItalic =
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
    std::string & strNormal = std::get<0>(formatting);
    std::string & strBold = std::get<1>(formatting);
    std::string & strItalic = std::get<2>(formatting);
    std::string & strBoldItalic = std::get<3>(formatting);
    std::string & strUnderline = std::get<4>(formatting);

    painter.SetFont(pFontNormal);

    painter.DrawMultiLineText(
        left_margin, bottom_margin, width, print_height - 12 * line,
        PoDoFo::PdfString((const PoDoFo::pdf_utf8 *)strNormal.c_str()), eAlignment,
        PoDoFo::ePdfVerticalAlignment_Top
    );

    painter.DrawMultiLineText(
        left_margin, bottom_margin, width, print_height - 12 * line,
        PoDoFo::PdfString((const PoDoFo::pdf_utf8 *)strUnderline.c_str()), eAlignment,
        PoDoFo::ePdfVerticalAlignment_Top
    );

    painter.SetFont(pFontBold);
    painter.DrawMultiLineText(
        left_margin, bottom_margin, width, print_height - 12 * line,
        PoDoFo::PdfString((const PoDoFo::pdf_utf8 *)strBold.c_str()), eAlignment,
        PoDoFo::ePdfVerticalAlignment_Top
    );

    painter.SetFont(pFontItalic);
    painter.DrawMultiLineText(
        left_margin, bottom_margin, width, print_height - 12 * line,
        PoDoFo::PdfString((const PoDoFo::pdf_utf8 *)strItalic.c_str()), eAlignment,
        PoDoFo::ePdfVerticalAlignment_Top
    );

    painter.SetFont(pFontBoldItalic);
    painter.DrawMultiLineText(
        left_margin, bottom_margin, width, print_height - 12 * line,
        PoDoFo::PdfString((const PoDoFo::pdf_utf8 *)strBoldItalic.c_str()), eAlignment,
        PoDoFo::ePdfVerticalAlignment_Top
    );

    ++line;
  }
}

}  // namespace

bool ftn2pdf(const std::string & fn, const std::string & input) {
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

  PoDoFo::PdfPage * pPage =
      document.CreatePage(PoDoFo::PdfPage::CreateStandardPageSize(PoDoFo::ePdfPageSize_Letter));

  if (!pPage) {
    PODOFO_RAISE_ERROR(PoDoFo::ePdfError_InvalidHandle);
  }

  PoDoFo::PdfPainter painter;
  painter.SetPage(pPage);

  PoDoFo::PdfFont * pFontNormal = document.CreateFont(PODOFO_HPDF_FONT_COURIER);
  pFontNormal->SetFontSize(12.0);
  painter.SetFont(pFontNormal);

  // process script
  Fountain::Script script(input);

  const int flags = Fountain::ScriptNodeType::ftnContinuation |
                    Fountain::ScriptNodeType::ftnKeyValue |
                    Fountain::ScriptNodeType::ftnUnknown;

  // Title page
  if (script.metadata.find("title") != script.metadata.end()) {
    std::string strText = script.metadata["title"];
    decode_entities_inplace(strText);
    replace_all_inplace(strText, "*", "");
    replace_all_inplace(strText, "_", "");

    int line = 18 - pdfTextLines(strText, width_dialog).size();
    pdfTextAdd(
        document, painter, "<u>" + to_upper(strText) + "</u>", "normal", line, 468, 72, 72, 648,
        PoDoFo::ePdfAlignment_Center
    );
    line += pdfTextLines(strText, width_dialog).size() + 4;

    if (!script.metadata["author"].empty()) {
      if (!script.metadata["credit"].empty()) {
        strText = to_lower(script.metadata["credit"]);
        decode_entities_inplace(strText);
        pdfTextAdd(
            document, painter, strText, "normal", line, 468, 72, 72, 648,
            PoDoFo::ePdfAlignment_Center
        );
        line += pdfTextLines(strText, width_dialog).size() + 1;
      } else {
        strText = "Written by";
        pdfTextAdd(
            document, painter, strText, "normal", line, 468, 72, 72, 648,
            PoDoFo::ePdfAlignment_Center
        );
        line += 2;
      }

      strText = script.metadata["author"];
      decode_entities_inplace(strText);
      pdfTextAdd(
          document, painter, strText, "normal", line, 468, 72, 72, 648,
          PoDoFo::ePdfAlignment_Center
      );
      line += pdfTextLines(strText, width_dialog).size() + 4;
    }

    if (!script.metadata["source"].empty()) {
      strText = script.metadata["source"];
      decode_entities_inplace(strText);
      pdfTextAdd(
          document, painter, strText, "normal", line, 468, 72, 72, 648,
          PoDoFo::ePdfAlignment_Center
      );
      line += pdfTextLines(strText, width_dialog).size() + 1;
    }

    if (!script.metadata["contact"].empty()) {
      strText = script.metadata["contact"];
      decode_entities_inplace(strText);

      int text_lines = pdfTextLines(strText, width_dialog).size();
      line = text_lines < 3 ? 51 : 54 - text_lines;
      pdfTextAdd(
          document, painter, strText, "normal", line, 468, 72, 72, 648,
          PoDoFo::ePdfAlignment_Left
      );
    } else if (!script.metadata["copyright"].empty()) {
      strText = "Copyright " + script.metadata["copyright"];
      decode_entities_inplace(strText);
      replace_all_inplace(strText, "(c)", "©");

      int text_lines = pdfTextLines(strText, width_dialog).size();
      line = text_lines < 3 ? 51 : 54 - text_lines;
      pdfTextAdd(
          document, painter, strText, "normal", line, 468, 72, 72, 648,
          PoDoFo::ePdfAlignment_Left
      );
    }

    if (!script.metadata["notes"].empty()) {
      strText = script.metadata["notes"];
      decode_entities_inplace(strText);

      int text_lines = pdfTextLines(strText, width_dialog).size();
      line -= (text_lines + 2);
      pdfTextAdd(
          document, painter, strText, "normal", line, 468, 72, 72, 648,
          PoDoFo::ePdfAlignment_Right
      );
    }

    painter.FinishPage();
    pPage =
        document.CreatePage(PoDoFo::PdfPage::CreateStandardPageSize(PoDoFo::ePdfPageSize_Letter)
        );
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
            pdfTextAdd(
                document, painter, outputDialog, "normal", LineNumber, width_dialog,
                margin_dialog
            );
            outputDialog.clear();
            dialog_state = 0;
            LineNumber += textLines;
          } else {
            LineNumber += textLines;
          }
        } else if (dialog_state == 2) {
          // Don't write DialogLeft without DialogRight
        } else if (dialog_state == 3) {
          int textLines_right = pdfTextLines(outputDialogRight, width_dialog_dual).size();
          int textLines_left = pdfTextLines(outputDialogLeft, width_dialog_dual).size();
          int textLines = std::max<int>(textLines_left, textLines_right);
          if (LineNumber + textLines <= lines_per_page) {
            pdfTextAdd(
                document, painter, outputDialogRight, "normal", LineNumber, width_dialog_dual,
                margin_dialog_right
            );
            pdfTextAdd(
                document, painter, outputDialogLeft, "normal", LineNumber, width_dialog_dual,
                margin_dialog_left
            );
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
        buffer = std::string(line_char_length - buffer.length() + 1, ' ') + buffer;

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
          PoDoFo::PdfPage::CreateStandardPageSize(PoDoFo::ePdfPageSize_Letter)
      );
      if (!pPage) {
        PODOFO_RAISE_ERROR(PoDoFo::ePdfError_InvalidHandle);
      }
      painter.SetPage(pPage);
      ++PageNumber;

      // Add page number
      std::string PageNumberText = std::to_string(PageNumber);
      PageNumberText = std::string(line_char_length - PageNumberText.length() - 1, ' ') +
                       PageNumberText + '.';
      pdfTextAdd(document, painter, PageNumberText, "normal", -3);

      // Add overflow from previous page
      if (dialog_state == 1 && !outputDialog.empty()) {
        int textLines = pdfTextLines(outputDialog, width_dialog).size();
        pdfTextAdd(
            document, painter, outputDialog, "normal", LineNumber, width_dialog, margin_dialog
        );
        outputDialog.clear();
        dialog_state = 0;
        LineNumber += textLines + 1;
      } else if (dialog_state == 2) {
        // Don't write DialogLeft without DialogRight
      } else if (dialog_state == 3 && (!outputDialogLeft.empty() || !outputDialogRight.empty())) {
        int textLines_left = pdfTextLines(outputDialogLeft, width_dialog_dual).size();
        int textLines_right = pdfTextLines(outputDialogRight, width_dialog_dual).size();
        int textLines = std::max<int>(textLines_left, textLines_right);

        pdfTextAdd(
            document, painter, outputDialogLeft, "normal", LineNumber, width_dialog_dual,
            margin_dialog_left
        );
        pdfTextAdd(
            document, painter, outputDialogRight, "normal", LineNumber, width_dialog_dual,
            margin_dialog_right
        );
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

}  // namespace Fountain
