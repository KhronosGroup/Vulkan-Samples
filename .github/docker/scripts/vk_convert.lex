%option 8bit noyywrap yylineno stack

/* Copyright (c) 2021, Arm Limited and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

%{
  #include <algorithm>
  #include <iostream>
  #include <cctype>
  #include <regex>
  #include <vector>
  #include <fstream>

std::vector<std::string> ignore_case;
bool loaded = false;

void load_ignore_cases() {
	std::ifstream file;
	file.exceptions(std::ifstream::badbit);
	try {
		file.open("./.snakeignore");
		std::string line;
		while(std::getline(file, line)) {
			ignore_case.push_back(line);
		}
	}
	catch(const std::ifstream::failure &e) {
		std::cout << "FAIL\n";
		exit(EXIT_FAILURE);
	}	

	file.close();
	loaded = true;
}

bool include(const std::string &s) {
 	if(!loaded) {
		load_ignore_cases();
	} 
	std::regex r("\\*|\n$");
	for (unsigned int i = 0; i < ignore_case.size(); i++) {
		if(std::regex_search(ignore_case[i], r)  && s.substr(0, ignore_case[i].size() - 2) == ignore_case[i].substr(0, ignore_case[i].size() - 2)) {
			return true;
		} else if (ignore_case[i] == s) {
			return true;
		}
	}
	return false;
 }

  bool first_cap(const std::string &s) {
	return s.size() && std::isupper(s[0]);
  }

  bool all_caps(const std::string &s)
  {
	return std::none_of(s.begin(), s.end(), ::islower);
  }

  // Convert lowerCamelCase and UpperCamelCase strings to lower_with_underscore.
  std::string convert(std::string &&camelCase)
  {
	std::string str(1, camelCase[0]);
	// First place underscores between contiguous lower and upper case letters.
	// For example, `_LowerCamelCase` becomes `_Lower_Camel_Case`.
	for (auto it = camelCase.begin() + 1; it != camelCase.end(); ++it)
	{
	  if (isupper(*it) && *(it-1) != '_' && islower(*(it-1)))
	  {
		str += "_";
	  }
	  str += *it;
	}

	// Then convert it to lower case.
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);

	return str;
  }

%}
%x X_STRING X_COMMENT X_INCLUDE
%%

\"                          ECHO; yy_push_state(X_STRING);
<X_STRING>\\\"              ECHO;
<X_STRING>\\\\              ECHO;
<X_STRING>\"                ECHO; yy_pop_state();
<X_STRING>.                 ECHO;

"//".*$                     ECHO;

"/*"                        ECHO; yy_push_state(X_COMMENT);
<X_COMMENT>"/*"             ECHO; yy_push_state(X_COMMENT);
<X_COMMENT>"*/"             ECHO; yy_pop_state();
<X_COMMENT>.|\n             ECHO;

#include                    ECHO; yy_push_state(X_INCLUDE);

<INITIAL,X_INCLUDE>[a-zA-Z_][a-zA-Z0-9_]*     {
							 std::string id(yytext);
							 if (first_cap(id) || all_caps(id) || include(id))
							   std::cout << id << std::flush;
							 else
							   std::cout << convert(std::move(id)) << std::flush;
						   }

<X_INCLUDE>\n              ECHO; yy_pop_state();

.|\n                       ECHO;

%%

int main(int argc, char **argv)
{
	yyFlexLexer tmp;
	return tmp.yylex();
}