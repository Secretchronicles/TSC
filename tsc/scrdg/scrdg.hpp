/***************************************************************************
 * scrdg.hpp - Scripting API Documentation Generator (header)
 *
 * Copyright © 2005 - 2011 Florian Richter
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

#include <string>
#include <initializer_list>
#include <boost/filesystem.hpp>

struct ClassDoc
{
    std::string name;
    std::string documentation;
};

struct ModuleDoc
{
    std::string name;
    std::string documentation;
};

struct MethodDoc
{
    std::string name;
    std::string classname;
    bool is_instance_method;
    std::vector<std::string> call_seqs;
    std::string documentation;
};

class Parser
{
public:
    Parser(std::string name, boost::filesystem::path source_directory, std::initializer_list<std::string> extensions);
    virtual ~Parser();

    void Parse();
    void PrintSummary();

    const inline std::vector<ClassDoc>&  GetClasses() { return m_classes; }
    const inline std::vector<ModuleDoc>& GetModules() { return m_modules; }
    const inline std::vector<MethodDoc>& GetMethods() { return m_methods; }
private:
    std::string m_parser_name;
    std::vector<std::string> m_file_extensions;
protected:
    boost::filesystem::path m_source_dir;
    size_t m_lino;
    std::vector<ClassDoc> m_classes;
    std::vector<ModuleDoc> m_modules;
    std::vector<MethodDoc> m_methods;

    virtual void parse_file(const boost::filesystem::path& file) = 0;
    void parse_doctext(std::string text);
    void parse_doctype_class(const std::string& classname, const std::string& text);
    void parse_doctype_module(const std::string& modulename, const std::string& text);
    void parse_doctype_method(const std::string& methodname, const std::string& text);
};

// This parser extracts API documentation from the scipting core C++
// source code files under src/scripting.
class CppParser: public Parser
{
public:
    CppParser(boost::filesystem::path source_directory);
private:
    virtual void parse_file(const boost::filesystem::path& file);

    bool m_docblock_open;
    std::string m_doctext;
};

// This parser extracts API documentation from the scripting
// standard library (SSL) Ruby source code files under data/scripting.
class RubyParser: public Parser
{
public:
    RubyParser(boost::filesystem::path source_directory);
private:
    virtual void parse_file(const boost::filesystem::path& file);

    size_t m_leading_spaces;
    bool m_docblock_open;
    std::string m_doctext;
};

// This generator created HTML pages from the information harvested
// with the two parsers defined above.
class Generator
{
public:
    Generator(boost::filesystem::path output_dir,
              boost::filesystem::path template_file,
              boost::filesystem::path index_file,
              const std::string& tsc_version,
              const std::string& tsc_gitrevision,
              const std::vector<ClassDoc>& classes,
              const std::vector<ModuleDoc>& modules,
              const std::vector<MethodDoc>& methods);

    void Generate();

    static std::string MakeDocFilename(std::string str);
    static std::string MakeMethodId(bool cmethod, std::string str);
private:
    void generate_classmod(const std::string& type, const std::string& name, const std::string& documentation);
    void generate_class(const ClassDoc& klass);
    void generate_module(const ModuleDoc& mod);
    void generate_indexfile();
    void filter_methods(const std::string& classmodname, std::vector<MethodDoc>& cmethods, std::vector<MethodDoc>& imethods);
    std::string tsc_version_str();

    const std::vector<ClassDoc>&  m_classes;
    const std::vector<ModuleDoc>& m_modules;
    const std::vector<MethodDoc>& m_methods;
    boost::filesystem::path m_output_dir;
    boost::filesystem::path m_index_file;
    const std::string& m_tsc_version;
    const std::string& m_tsc_gitrevision;
    std::string m_template;
};

std::string strip(std::string str);
