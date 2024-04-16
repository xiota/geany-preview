/*
 * Fountain Screenplay Processor - auxiliary functions
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

#include <cstring>
#include <iterator>
#include <regex>
#include <string>
#include <vector>

#define DEBUG printf("%s\n\t%d\n\t%s\n\n", __FILE__, __LINE__, __FUNCTION__);

#define FOUNTAIN_WHITESPACE " \t\n\r\f\v"

std::string &ltrim_inplace(std::string &s, const char *t = FOUNTAIN_WHITESPACE);
std::string &rtrim_inplace(std::string &s, const char *t = FOUNTAIN_WHITESPACE);
std::string &trim_inplace(std::string &s, const char *t = FOUNTAIN_WHITESPACE);

std::string &replace_all_inplace(std::string &subject,
                                 const std::string_view &search,
                                 const std::string_view &replace);

std::string ws_ltrim(std::string s);
std::string ws_rtrim(std::string s);
std::string ws_trim(std::string s);

std::string replace_all(std::string subject, const std::string_view &search,
                        const std::string_view &replace);

bool begins_with(const std::string &input, const std::string &match);

std::vector<std::string> split_string(const std::string_view &str,
                                      const std::string_view &delimiter = " ");

std::vector<std::string> split_lines(const std::string_view &s);

std::string &to_upper_inplace(std::string &s);
std::string &to_lower_inplace(std::string &s);

std::string to_upper(std::string s);
std::string to_lower(std::string s);

bool is_upper(const std::string_view &s);

std::string &encode_entities_inplace(std::string &input,
                                     const bool bProcessAllEntities = false);
std::string encode_entities(std::string input,
                            const bool bProcessAllEntities = false);

std::string &decode_entities_inplace(std::string &input);
std::string decode_entities(std::string input);

std::string cstr_assign(char *input);

std::vector<std::string> cstrv_assign(char **input);
std::vector<std::string> cstrv_copy(const char *const *input);
std::vector<char *> cstrv_get(const std::vector<std::string> input);

std::string file_get_contents(const std::string &filename);

bool file_set_contents(const std::string &filename,
                       const std::string &contents);

void print_regex_error(std::regex_error &e, const char *file, const int line);
