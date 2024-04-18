/*
 * SPDX-FileCopyrightText: Copyright 2021-2024 xiota
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

namespace Fountain {

namespace {  // PDF export - using PoDoFo 0.10.x

#define PODOFO_RAISE_ERROR(code) throw ::PoDoFo::PdfError(code, __FILE__, __LINE__)

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
    PoDoFo::PdfDocument & document, PoDoFo::PdfPainter & painter, const std::string & text,
    const std::string & font, int line, const int width = 432, const int left_margin = 108,
    const int bottom_margin = 72, const int print_height = 648,
    const PoDoFo::PdfHorizontalAlignment horizontalAlignment =
        PoDoFo::PdfHorizontalAlignment::Left
) {
  // set alignment parameters
  PoDoFo::PdfDrawTextMultiLineParams drawParams;
  drawParams.HorizontalAlignment = horizontalAlignment;
  drawParams.VerticalAlignment = PoDoFo::PdfVerticalAlignment::Top;

  // set font search parameters, needed for bold, italic, etc
  PoDoFo::PdfFontSearchParams fontSearchParams;
  fontSearchParams.MatchBehavior = PoDoFo::PdfFontMatchBehaviorFlags::NormalizePattern;

  if (font == "title") {
    PoDoFo::PdfFont * pFontCustom =
        document.GetFonts().SearchFont("Alegreya SC Medium", fontSearchParams);

    if (pFontCustom == nullptr) {
      PODOFO_RAISE_ERROR(PoDoFo::PdfErrorCode::InvalidHandle);
    }

    painter.TextState.SetFont(*pFontCustom, 24);

    painter.DrawTextMultiLine(
        text, left_margin, bottom_margin, width, print_height - 12 * line, drawParams
    );
    return;
  }

  PoDoFo::PdfFont * pFontNormal = nullptr;
  if (font == "bold" || font == "sceneheader") {
    pFontNormal = document.GetFonts().SearchFont("Courier Prime Bold", fontSearchParams);
  } else if (font == "italic" || font == "lyric") {
    pFontNormal = document.GetFonts().SearchFont("Courier Prime Italic", fontSearchParams);
  } else {
    pFontNormal = document.GetFonts().SearchFont("Courier Prime", fontSearchParams);
  }

  PoDoFo::PdfFont * pFontItalic =
      document.GetFonts().SearchFont("Courier Prime Italic", fontSearchParams);

  PoDoFo::PdfFont * pFontBold =
      document.GetFonts().SearchFont("Courier Prime Bold", fontSearchParams);

  PoDoFo::PdfFont * pFontBoldItalic =
      document.GetFonts().SearchFont("Courier Prime Bold Italic", fontSearchParams);

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

    painter.TextState.SetFont(*pFontNormal, 12);
    painter.DrawTextMultiLine(
        strNormal, left_margin, bottom_margin, width, (print_height - 12 * line), drawParams
    );

    painter.DrawTextMultiLine(
        strUnderline, left_margin, bottom_margin, width, (print_height - 12 * line), drawParams
    );

    painter.TextState.SetFont(*pFontBold, 12);
    painter.DrawTextMultiLine(
        strBold, left_margin, bottom_margin, width, (print_height - 12 * line), drawParams
    );

    painter.TextState.SetFont(*pFontItalic, 12);
    painter.DrawTextMultiLine(
        strItalic, left_margin, bottom_margin, width, (print_height - 12 * line), drawParams
    );

    painter.TextState.SetFont(*pFontBoldItalic, 12);
    painter.DrawTextMultiLine(
        strBoldItalic, left_margin, bottom_margin, width, (print_height - 12 * line), drawParams
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

  // PDF document
  // PoDoFo::PdfStreamedDocument pdf_document(fn);
  PoDoFo::PdfMemDocument pdf_document;

  // PDF page size
  PoDoFo::Rect pdf_size = PoDoFo::PdfPage::CreateStandardPageSize(PoDoFo::PdfPageSize::Letter);
  PoDoFo::PdfPage * pdf_page = &pdf_document.GetPages().CreatePage(pdf_size);

  // PDF font
  PoDoFo::PdfFont * pdf_font = pdf_document.GetFonts().SearchFont("Courier Prime");

  PoDoFo::PdfPainter pdf_painter;
  pdf_painter.SetCanvas(*pdf_page);
  pdf_painter.TextState.SetFont(*pdf_font, 12);

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
        pdf_document, pdf_painter, "<u>" + to_upper(strText) + "</u>", "normal", line, 468, 72,
        72, 648, PoDoFo::PdfHorizontalAlignment::Center
    );
    line += pdfTextLines(strText, width_dialog).size() + 4;

    if (!script.metadata["author"].empty()) {
      if (!script.metadata["credit"].empty()) {
        strText = to_lower(script.metadata["credit"]);
        decode_entities_inplace(strText);
        pdfTextAdd(
            pdf_document, pdf_painter, strText, "normal", line, 468, 72, 72, 648,
            PoDoFo::PdfHorizontalAlignment::Center
        );
        line += pdfTextLines(strText, width_dialog).size() + 1;
      } else {
        strText = "Written by";
        pdfTextAdd(
            pdf_document, pdf_painter, strText, "normal", line, 468, 72, 72, 648,
            PoDoFo::PdfHorizontalAlignment::Center
        );
        line += 2;
      }

      strText = script.metadata["author"];
      decode_entities_inplace(strText);
      pdfTextAdd(
          pdf_document, pdf_painter, strText, "normal", line, 468, 72, 72, 648,
          PoDoFo::PdfHorizontalAlignment::Center
      );
      line += pdfTextLines(strText, width_dialog).size() + 4;
    }

    if (!script.metadata["source"].empty()) {
      strText = script.metadata["source"];
      decode_entities_inplace(strText);
      pdfTextAdd(
          pdf_document, pdf_painter, strText, "normal", line, 468, 72, 72, 648,
          PoDoFo::PdfHorizontalAlignment::Center
      );
      line += pdfTextLines(strText, width_dialog).size() + 1;
    }

    if (!script.metadata["contact"].empty()) {
      strText = script.metadata["contact"];
      decode_entities_inplace(strText);

      int text_lines = pdfTextLines(strText, width_dialog).size();
      line = text_lines < 3 ? 51 : 54 - text_lines;
      pdfTextAdd(
          pdf_document, pdf_painter, strText, "normal", line, 468, 72, 72, 648,
          PoDoFo::PdfHorizontalAlignment::Left
      );
    } else if (!script.metadata["copyright"].empty()) {
      strText = "Copyright " + script.metadata["copyright"];
      decode_entities_inplace(strText);
      replace_all_inplace(strText, "(c)", "©");

      int text_lines = pdfTextLines(strText, width_dialog).size();
      line = text_lines < 3 ? 51 : 54 - text_lines;
      pdfTextAdd(
          pdf_document, pdf_painter, strText, "normal", line, 468, 72, 72, 648,
          PoDoFo::PdfHorizontalAlignment::Left
      );
    }

    if (!script.metadata["notes"].empty()) {
      strText = script.metadata["notes"];
      decode_entities_inplace(strText);

      int text_lines = pdfTextLines(strText, width_dialog).size();
      line -= (text_lines + 2);
      pdfTextAdd(
          pdf_document, pdf_painter, strText, "normal", line, 468, 72, 72, 648,
          PoDoFo::PdfHorizontalAlignment::Right
      );
    }

    pdf_painter.FinishDrawing();
    pdf_page = &pdf_document.GetPages().CreatePage(pdf_size);
    pdf_painter.SetCanvas(*pdf_page);
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
                pdf_document, pdf_painter, outputDialog, "normal", LineNumber, width_dialog,
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
                pdf_document, pdf_painter, outputDialogRight, "normal", LineNumber,
                width_dialog_dual, margin_dialog_right
            );
            pdfTextAdd(
                pdf_document, pdf_painter, outputDialogLeft, "normal", LineNumber,
                width_dialog_dual, margin_dialog_left
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
          pdfTextAdd(pdf_document, pdf_painter, buffer, "sceneheader", LineNumber);
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
          pdfTextAdd(pdf_document, pdf_painter, buffer, "normal", LineNumber);
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
          pdfTextAdd(pdf_document, pdf_painter, buffer, "normal", LineNumber);
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
          pdfTextAdd(pdf_document, pdf_painter, buffer, "normal", LineNumber);
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
            pdfTextAdd(pdf_document, pdf_painter, buffer, "lyric", LineNumber);
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
      pdf_painter.FinishDrawing();
      LineNumber = 0;

      pdf_page = &pdf_document.GetPages().CreatePage(pdf_size);
      pdf_painter.SetCanvas(*pdf_page);
      ++PageNumber;

      // Add page number
      std::string PageNumberText = std::to_string(PageNumber);
      PageNumberText = std::string(line_char_length - PageNumberText.length() - 1, ' ') +
                       PageNumberText + '.';
      pdfTextAdd(pdf_document, pdf_painter, PageNumberText, "normal", -3);

      // Add overflow from previous page
      if (dialog_state == 1 && !outputDialog.empty()) {
        int textLines = pdfTextLines(outputDialog, width_dialog).size();
        pdfTextAdd(
            pdf_document, pdf_painter, outputDialog, "normal", LineNumber, width_dialog,
            margin_dialog
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
            pdf_document, pdf_painter, outputDialogLeft, "normal", LineNumber,
            width_dialog_dual, margin_dialog_left
        );
        pdfTextAdd(
            pdf_document, pdf_painter, outputDialogRight, "normal", LineNumber,
            width_dialog_dual, margin_dialog_right
        );
        outputDialogLeft.clear();
        outputDialogRight.clear();
        dialog_state = 0;
        LineNumber += textLines + 1;
      } else if (!output.empty()) {
        if (node.type == ScriptNodeType::ftnSceneHeader) {
          int textLines = pdfTextLines(output).size();
          pdfTextAdd(pdf_document, pdf_painter, output, "sceneheader", LineNumber);
          output.clear();
          LineNumber += textLines;
        } else {
          int textLines = pdfTextLines(output).size();
          pdfTextAdd(pdf_document, pdf_painter, output, "normal", LineNumber);
          output.clear();
          LineNumber += textLines;
        }
      }
    }
  }

  pdf_painter.FinishDrawing();

  pdf_document.GetMetadata().SetCreator(PoDoFo::PdfString("Geany Preview Plugin"));

  if (!script.metadata["author"].empty()) {
    pdf_document.GetMetadata().SetAuthor(PoDoFo::PdfString(script.metadata["author"]));
  }

  if (!script.metadata["title"].empty()) {
    pdf_document.GetMetadata().SetTitle(PoDoFo::PdfString(script.metadata["title"]));
  }

  pdf_document.Save(fn);
  return true;
}

}  // namespace Fountain
