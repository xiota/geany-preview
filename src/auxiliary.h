/* -*- C++ -*-
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

std::string &ltrim(std::string &s, char const *t = " \t\n\r\f\v");
std::string &rtrim(std::string &s, char const *t = " \t\n\r\f\v");
std::string &trim(std::string &s, char const *t = " \t\n\r\f\v");

std::string ws_ltrim(std::string s);
std::string ws_rtrim(std::string s);
std::string ws_trim(std::string s);

bool begins_with(std::string const &input, std::string const &match);

std::vector<std::string> split_string(std::string const &str,
                                      std::string const &delimiter);

std::vector<std::string> split_lines(std::string const &s);

std::string to_upper(std::string s);

std::string to_lower(std::string s);

bool is_upper(std::string const &s);

void print_regex_error(std::regex_error &e);
