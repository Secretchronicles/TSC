/***************************************************************************
 * file_parser.h
 *
 * Copyright © 2012-2020 The TSC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TSC_FILE_PARSER_HPP
#define TSC_FILE_PARSER_HPP

#include "../core/global_game.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** *** cFile_parser *** *** *** *** *** *** *** *** *** */

// Base class for parsing text files
    class cFile_parser {
    public:
        cFile_parser(void);
        virtual ~cFile_parser(void);

        // Parses the given file
        bool Parse(const boost::filesystem::path& filename);

        // Tokenize a line
        bool Parse_Line(std::string str_line, int line_num);

        // Handle one tokenized line
        virtual bool HandleMessage(const std::string* parts, unsigned int count, unsigned int line);

        // data filename
        boost::filesystem::path data_file;
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif

