// -*- Mode: c++; c-basic-offset: 4; tab-width: 4; -*-

/******************************************************************************
 *
 *  file:  DocBookOutput.h
 *
 *  Copyright (c) 2004, Michael E. Smoot
 *  Copyright (c) 2017, Google LLC
 *  All rights reserved.
 *
 *  See the file COPYING in the top directory of this distribution for
 *  more information.
 *
 *  THE SOFTWARE IS PROVIDED _AS IS_, WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/

#ifndef TCLAP_DOC_BOOK_OUTPUT_H
#define TCLAP_DOC_BOOK_OUTPUT_H

#include <tclap/Arg.h>
#include <tclap/CmdLineInterface.h>
#include <tclap/CmdLineOutput.h>

#include <algorithm>
#include <iostream>
#include <list>
#include <string>
#include <vector>

namespace TCLAP {

/**
 * A class that generates DocBook output for usage() method for the
 * given CmdLine and its Args.
 */
class DocBookOutput : public CmdLineOutput {
public:
    /**
     * Prints the usage to stdout.  Can be overridden to
     * produce alternative behavior.
     * \param c - The CmdLine object the output is generated for.
     */
    virtual void usage(CmdLineInterface &c);

    /**
     * Prints the version to stdout. Can be overridden
     * to produce alternative behavior.
     * \param c - The CmdLine object the output is generated for.
     */
    virtual void version(CmdLineInterface &c);

    /**
     * Prints (to stderr) an error message, short usage
     * Can be overridden to produce alternative behavior.
     * \param c - The CmdLine object the output is generated for.
     * \param e - The ArgException that caused the failure.
     */
    virtual void failure(CmdLineInterface &c, ArgException &e);

    DocBookOutput() : theDelimiter('=') {}

protected:
    /**
     * Substitutes the char r for string x in string s.
     * \param s - The string to operate on.
     * \param r - The char to replace.
     * \param x - What to replace r with.
     */
    void substituteSpecialChars(std::string &s, char r,
                                const std::string &x) const;
    void removeChar(std::string &s, char r) const;

    void printShortArg(Arg *it, bool required);
    void printLongArg(const ArgGroup &it) const;

    char theDelimiter;
};

inline void DocBookOutput::version(CmdLineInterface &_cmd) {
    std::cout << _cmd.getVersion() << std::endl;
}

namespace internal {
const char *GroupChoice(const ArgGroup &group) {
    if (!group.showAsGroup()) {
        return "plain";
    }

    if (group.isRequired()) {
        return "req";
    }

    return "opt";
}
}  // namespace internal

inline void DocBookOutput::usage(CmdLineInterface &_cmd) {
    std::list<ArgGroup *> argSets = _cmd.getArgGroups();
    std::string progName = _cmd.getProgramName();
    std::string xversion = _cmd.getVersion();
    theDelimiter = _cmd.getDelimiter();

    std::cout << "<?xml version='1.0'?>\n";
    std::cout
        << "<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.2//EN\"\n";
    std::cout
        << "\t\"http://www.oasis-open.org/docbook/xml/4.2/docbookx.dtd\">\n\n";

    std::cout << "<refentry>\n";

    std::cout << "<refmeta>\n";
    std::cout << "<refentrytitle>" << progName << "</refentrytitle>\n";
    std::cout << "<manvolnum>1</manvolnum>\n";
    std::cout << "</refmeta>\n";

    std::cout << "<refnamediv>\n";
    std::cout << "<refname>" << progName << "</refname>\n";
    std::cout << "<refpurpose>" << _cmd.getMessage() << "</refpurpose>\n";
    std::cout << "</refnamediv>\n";

    std::cout << "<refsynopsisdiv>\n";
    std::cout << "<cmdsynopsis>\n";

    std::cout << "<command>" << progName << "</command>\n";

    for (std::list<ArgGroup *>::iterator sit = argSets.begin();
         sit != argSets.end(); ++sit) {
        int visible = CountVisibleArgs(**sit);
        if (visible > 1) {
            std::cout << "<group choice='" << internal::GroupChoice(**sit)
                      << "'>\n";
        }
        for (ArgGroup::iterator it = (*sit)->begin(); it != (*sit)->end();
             ++it) {
            if (!(*it)->visibleInHelp()) {
                continue;
            }

            printShortArg(*it, (*it)->isRequired() ||
                                   (visible == 1 && (**sit).isRequired()));
        }
        if (visible > 1) {
            std::cout << "</group>\n";
        }
    }

    std::cout << "</cmdsynopsis>\n";
    std::cout << "</refsynopsisdiv>\n";

    std::cout << "<refsect1>\n";
    std::cout << "<title>Description</title>\n";
    std::cout << "<para>\n";
    std::cout << _cmd.getMessage() << '\n';
    std::cout << "</para>\n";
    std::cout << "</refsect1>\n";

    std::cout << "<refsect1>\n";
    std::cout << "<title>Options</title>\n";

    std::cout << "<variablelist>\n";

    for (std::list<ArgGroup *>::iterator sit = argSets.begin();
         sit != argSets.end(); ++sit) {
        printLongArg(**sit);
    }

    std::cout << "</variablelist>\n";
    std::cout << "</refsect1>\n";

    std::cout << "<refsect1>\n";
    std::cout << "<title>Version</title>\n";
    std::cout << "<para>\n";
    std::cout << xversion << '\n';
    std::cout << "</para>\n";
    std::cout << "</refsect1>\n";

    std::cout << "</refentry>" << std::endl;
}

inline void DocBookOutput::failure(CmdLineInterface &_cmd, ArgException &e) {
    static_cast<void>(_cmd);  // unused
    std::cout << e.what() << std::endl;
    throw ExitException(1);
}

inline void DocBookOutput::substituteSpecialChars(std::string &s, char r,
                                                  const std::string &x) const {
    size_t p;
    while ((p = s.find_first_of(r)) != std::string::npos) {
        s.erase(p, 1);
        s.insert(p, x);
    }
}

inline void DocBookOutput::removeChar(std::string &s, char r) const {
    size_t p;
    while ((p = s.find_first_of(r)) != std::string::npos) {
        s.erase(p, 1);
    }
}

inline void DocBookOutput::printShortArg(Arg *a, bool required) {
    std::string lt = "&lt;";
    std::string gt = "&gt;";

    std::string id = a->shortID();
    substituteSpecialChars(id, '<', lt);
    substituteSpecialChars(id, '>', gt);
    removeChar(id, '[');
    removeChar(id, ']');

    std::string choice = "opt";
    if (required) {
        choice = "plain";
    }

    std::cout << "<arg choice='" << choice << '\'';
    if (a->acceptsMultipleValues()) std::cout << " rep='repeat'";

    std::cout << '>';
    if (!a->getFlag().empty())
        std::cout << a->flagStartChar() << a->getFlag();
    else
        std::cout << a->nameStartString() << a->getName();
    if (a->isValueRequired()) {
        std::string arg = a->shortID();
        removeChar(arg, '[');
        removeChar(arg, ']');
        removeChar(arg, '<');
        removeChar(arg, '>');
        removeChar(arg, '.');
        arg.erase(0, arg.find_last_of(theDelimiter) + 1);
        std::cout << theDelimiter;
        std::cout << "<replaceable>" << arg << "</replaceable>";
    }
    std::cout << "</arg>" << std::endl;
}

inline void DocBookOutput::printLongArg(const ArgGroup &group) const {
    const std::string lt = "&lt;";
    const std::string gt = "&gt;";

    bool forceRequired = group.isRequired() && CountVisibleArgs(group) == 1;
    for (ArgGroup::const_iterator it = group.begin(); it != group.end(); ++it) {
        Arg &a = **it;
        if (!a.visibleInHelp()) {
            continue;
        }

        std::string desc = a.getDescription(forceRequired || a.isRequired());
        substituteSpecialChars(desc, '<', lt);
        substituteSpecialChars(desc, '>', gt);

        std::cout << "<varlistentry>\n";

        if (!a.getFlag().empty()) {
            std::cout << "<term>\n";
            std::cout << "<option>";
            std::cout << a.flagStartChar() << a.getFlag();
            std::cout << "</option>\n";
            std::cout << "</term>\n";
        }

        std::cout << "<term>\n";
        std::cout << "<option>";
        std::cout << a.nameStartString() << a.getName();
        if (a.isValueRequired()) {
            std::string arg = a.shortID();
            removeChar(arg, '[');
            removeChar(arg, ']');
            removeChar(arg, '<');
            removeChar(arg, '>');
            removeChar(arg, '.');
            arg.erase(0, arg.find_last_of(theDelimiter) + 1);
            std::cout << theDelimiter;
            std::cout << "<replaceable>" << arg << "</replaceable>";
        }

        std::cout << "</option>\n";
        std::cout << "</term>\n";

        std::cout << "<listitem>\n";
        std::cout << "<para>\n";
        std::cout << desc << '\n';
        std::cout << "</para>\n";
        std::cout << "</listitem>\n";

        std::cout << "</varlistentry>" << std::endl;
    }
}

}  // namespace TCLAP
#endif  // TCLAP_DOC_BOOK_OUTPUT_H
