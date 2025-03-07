/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2022 Cppcheck team.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//---------------------------------------------------------------------------
#ifndef preprocessorH
#define preprocessorH
//---------------------------------------------------------------------------

#include "config.h"

#include <atomic>
#include <simplecpp.h>
#include <istream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

class ErrorLogger;
class Settings;

/**
 * @brief A preprocessor directive
 * Each preprocessor directive (\#include, \#define, \#undef, \#if, \#ifdef, \#else, \#endif)
 * will be recorded as an instance of this class.
 *
 * file and linenr denote the location where where the directive is defined.
 *
 */

class CPPCHECKLIB Directive {
public:
    /** name of (possibly included) file where directive is defined */
    std::string file;

    /** line number in (possibly included) file where directive is defined */
    unsigned int linenr;

    /** the actual directive text */
    std::string str;

    /** record a directive (possibly filtering src) */
    Directive(const std::string &_file, const int _linenr, const std::string &_str);
};

/// @addtogroup Core
/// @{

/**
 * @brief The cppcheck preprocessor.
 * The preprocessor has special functionality for extracting the various ifdef
 * configurations that exist in a source file.
 */
class CPPCHECKLIB Preprocessor {
public:

    /**
     * Include file types.
     */
    enum HeaderTypes {
        NoHeader = 0,
        UserHeader,
        SystemHeader
    };

    /** character that is inserted in expanded macros */
    static char macroChar;

    explicit Preprocessor(Settings& settings, ErrorLogger *errorLogger = nullptr);
    virtual ~Preprocessor();

    static std::atomic<bool> missingIncludeFlag;
    static std::atomic<bool> missingSystemIncludeFlag;

    void inlineSuppressions(const simplecpp::TokenList &tokens);

    void setDirectives(const simplecpp::TokenList &tokens);
    void setDirectives(const std::list<Directive> &directives) {
        mDirectives = directives;
    }

    /** list of all directives met while preprocessing file */
    const std::list<Directive> &getDirectives() const {
        return mDirectives;
    }

    std::set<std::string> getConfigs(const simplecpp::TokenList &tokens) const;

    void handleErrors(const simplecpp::OutputList &outputList, bool throwError);

    bool loadFiles(const simplecpp::TokenList &rawtokens, std::vector<std::string> &files);

    void removeComments();

    void setPlatformInfo(simplecpp::TokenList *tokens) const;

    /**
     * Extract the code for each configuration
     * @param istr The (file/string) stream to read from.
     * @param result The map that will get the results
     * @param filename The name of the file to check e.g. "src/main.cpp"
     * @param includePaths List of paths where include files should be searched from,
     * single path can be e.g. in format "include/".
     * There must be a path separator at the end. Default parameter is empty list.
     * Note that if path from given filename is also extracted and that is used as
     * a last include path if include file was not found from earlier paths.
     */
    void preprocess(std::istream &istr, std::map<std::string, std::string> &result, const std::string &filename, const std::list<std::string> &includePaths = std::list<std::string>());

    /**
     * Extract the code for each configuration. Use this with getcode() to get the
     * file data for each individual configuration.
     *
     * @param srcCodeStream The (file/string) stream to read from.
     * @param processedFile Give reference to empty string as a parameter,
     * function will fill processed file here. Use this also as a filedata parameter
     * to getcode() if you received more than once configurations.
     * @param resultConfigurations List of configurations. Pass these one by one
     * to getcode() with processedFile.
     * @param filename The name of the file to check e.g. "src/main.cpp"
     * @param includePaths List of paths where include files should be searched from,
     * single path can be e.g. in format "include/".
     * There must be a path separator at the end. Default parameter is empty list.
     * Note that if path from given filename is also extracted and that is used as
     * a last include path if include file was not found from earlier paths.
     */
    void preprocess(std::istream &srcCodeStream, std::string &processedFile, std::list<std::string> &resultConfigurations, const std::string &filename, const std::list<std::string> &includePaths);

    simplecpp::TokenList preprocess(const simplecpp::TokenList &tokens1, const std::string &cfg, std::vector<std::string> &files, bool throwError = false);

    std::string getcode(const simplecpp::TokenList &tokens1, const std::string &cfg, std::vector<std::string> &files, const bool writeLocations);

    /**
     * Get preprocessed code for a given configuration
     * @param filedata file data including preprocessing 'if', 'define', etc
     * @param cfg configuration to read out
     * @param filename name of source file
     */
    std::string getcode(const std::string &filedata, const std::string &cfg, const std::string &filename);

    /**
     * make sure empty configuration macros are not used in code. the given code must be a single configuration
     * @param cfg configuration
     * @param macroUsageList macro usage list
     * @return true => configuration is valid
     */
    bool validateCfg(const std::string &cfg, const std::list<simplecpp::MacroUsage> &macroUsageList);
    void validateCfgError(const std::string &file, const unsigned int line, const std::string &cfg, const std::string &macro);

    /**
     * Calculate CRC32 checksum. Using toolinfo, tokens1, filedata.
     *
     * @param tokens1    Sourcefile tokens
     * @param toolinfo   Arbitrary extra toolinfo
     * @return CRC32 checksum
     */
    unsigned int calculateChecksum(const simplecpp::TokenList &tokens1, const std::string &toolinfo) const;

    void simplifyPragmaAsm(simplecpp::TokenList *tokenList) const;

private:

    static void simplifyPragmaAsmPrivate(simplecpp::TokenList *tokenList);

public:


    static void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings);

    void setFile0(const std::string &f) {
        mFile0 = f;
    }

    /**
     * dump all directives present in source file
     */
    void dump(std::ostream &out) const;

    void reportOutput(const simplecpp::OutputList &outputList, bool showerror);

private:
    void missingInclude(const std::string &filename, unsigned int linenr, const std::string &header, HeaderTypes headerType);
    void error(const std::string &filename, unsigned int linenr, const std::string &msg);

    Settings& mSettings;
    ErrorLogger *mErrorLogger;

    /** list of all directives met while preprocessing file */
    std::list<Directive> mDirectives;

    std::map<std::string, simplecpp::TokenList *> mTokenLists;

    /** filename for cpp/c file - useful when reporting errors */
    std::string mFile0;

    /** simplecpp tracking info */
    std::list<simplecpp::MacroUsage> mMacroUsage;
    std::list<simplecpp::IfCond> mIfCond;
};

/// @}
//---------------------------------------------------------------------------
#endif // preprocessorH
