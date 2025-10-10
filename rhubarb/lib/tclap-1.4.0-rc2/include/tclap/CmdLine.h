// -*- Mode: c++; c-basic-offset: 4; tab-width: 4; -*-

/******************************************************************************
 *
 *  file:  CmdLine.h
 *
 *  Copyright (c) 2003, Michael E. Smoot .
 *  Copyright (c) 2004, Michael E. Smoot, Daniel Aarno.
 *  Copyright (c) 2018, Google LLC
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

#ifndef TCLAP_CMD_LINE_H
#define TCLAP_CMD_LINE_H

#include <tclap/MultiSwitchArg.h>
#include <tclap/SwitchArg.h>
#include <tclap/UnlabeledMultiArg.h>
#include <tclap/UnlabeledValueArg.h>

#include <tclap/HelpVisitor.h>
#include <tclap/IgnoreRestVisitor.h>
#include <tclap/VersionVisitor.h>

#include <tclap/CmdLineOutput.h>
#include <tclap/StdOutput.h>

#include <tclap/Constraint.h>
#include <tclap/ValuesConstraint.h>

#include <tclap/ArgGroup.h>
#include <tclap/DeferDelete.h>

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <list>
#include <string>
#include <vector>

namespace TCLAP {

class StandaloneArgs : public AnyOf {
public:
    StandaloneArgs() {}

    ArgContainer &add(Arg *arg) {
        // std::cerr << "Adding " << arg->getName() << " to StandaloneArgs\n";
        for (iterator it = begin(); it != end(); it++) {
            if (*arg == **it) {
                throw SpecificationException(
                    "Argument with same flag/name already exists!",
                    arg->longID());
            }
        }

        _args.push_back(arg);

        return *this;
    }

    bool showAsGroup() const { return false; }
};

/**
 * The base class that manages the command line definition and passes
 * along the parsing to the appropriate Arg classes.
 */
class CmdLine : public CmdLineInterface {
protected:
    /**
     * The list of arguments that will be tested against the
     * command line.
     */
    std::list<Arg *> _argList;

    StandaloneArgs _standaloneArgs;
    StandaloneArgs _autoArgs;  // --help, --version, etc

    /**
     * Some args have set constraints on them (i.e., exactly or at
     * most one must be specified.
     */
    std::list<ArgGroup *> _argGroups;

    /**
     * The name of the program.  Set to argv[0].
     */
    std::string _progName;

    /**
     * A message used to describe the program.  Used in the usage output.
     */
    std::string _message;

    /**
     * The version to be displayed with the --version switch.
     */
    std::string _version;

    /**
     * The number of arguments that are required to be present on
     * the command line. This is set dynamically, based on the
     * Args added to the CmdLine object.
     */
    int _numRequired;

    /**
     * The character that is used to separate the argument flag/name
     * from the value.  Defaults to ' ' (space).
     */
    char _delimiter;

    /**
     * Add pointers that should be deleted as part of cleanup when
     * this object is destroyed.
     * @internal.
     */
    DeferDelete _deleteOnExit;

    /**
     * Default output handler if nothing is specified.
     */
    StdOutput _defaultOutput;

    /**
     * Object that handles all output for the CmdLine.
     */
    CmdLineOutput *_output;

    /**
     * Should CmdLine handle parsing exceptions internally?
     */
    bool _handleExceptions;

    /**
     * Throws an exception listing the missing args.
     */
    void missingArgsException(const std::list<ArgGroup *> &missing);

    /**
     * Checks whether a name/flag string matches entirely matches
     * the Arg::blankChar.  Used when multiple switches are combined
     * into a single argument.
     * \param s - The message to be used in the usage.
     */
    bool _emptyCombined(const std::string &s);

private:
    /**
     * Prevent accidental copying.
     */
    CmdLine(const CmdLine &rhs);
    CmdLine &operator=(const CmdLine &rhs);

    /**
     * Encapsulates the code common to the constructors
     * (which is all of it).
     */
    void _constructor();

    /**
     * Whether or not to automatically create help and version switches.
     */
    bool _helpAndVersion;

    /**
     * Whether or not to ignore unmatched args.
     */
    bool _ignoreUnmatched;

    /**
     * Ignoring arguments (e.g., after we have seen "--")
     */
    bool _ignoring;

public:
    /**
     * Command line constructor. Defines how the arguments will be
     * parsed.
     * \param message - The message to be used in the usage
     * output.
     * \param delimiter - The character that is used to separate
     * the argument flag/name from the value.  Defaults to ' ' (space).
     * \param version - The version number to be used in the
     * --version switch.
     * \param helpAndVersion - Whether or not to create the Help and
     * Version switches. Defaults to true.
     */
    CmdLine(const std::string &message, const char delimiter = ' ',
            const std::string &version = "none", bool helpAndVersion = true);

    /**
     * Deletes any resources allocated by a CmdLine object.
     */
    virtual ~CmdLine() {}

    /**
     * Adds an argument to the list of arguments to be parsed.
     *
     * @param a - Argument to be added.
     * @retval A reference to this so that add calls can be chained
     */
    ArgContainer &add(Arg &a);

    /**
     * An alternative add.  Functionally identical.
     *
     * @param a - Argument to be added.
     * @retval A reference to this so that add calls can be chained
     */
    ArgContainer &add(Arg *a);

    /**
     * Adds an argument group to the list of arguments to be parsed.
     *
     * All arguments in the group are added and the ArgGroup
     * object will validate that the input matches its
     * constraints.
     *
     * @param args - Argument group to be added.
     * @retval A reference to this so that add calls can be chained
     */
    ArgContainer &add(ArgGroup &args);

    // Internal, do not use
    void addToArgList(Arg *a);

    /**
     * \deprecated Use OneOf instead.
     */
    void xorAdd(Arg &a, Arg &b);

    /**
     * \deprecated Use OneOf instead.
     */
    void xorAdd(const std::vector<Arg *> &xors);

    /**
     * Parses the command line.
     * \param argc - Number of arguments.
     * \param argv - Array of arguments.
     */
    void parse(int argc, const char *const *argv);

    /**
     * Parses the command line.
     * \param args - A vector of strings representing the args.
     * args[0] is still the program name.
     */
    void parse(std::vector<std::string> &args);

    void setOutput(CmdLineOutput *co);

    std::string getVersion() const { return _version; }

    std::string getProgramName() const { return _progName; }

    // TOOD: Get rid of getArgList
    std::list<Arg *> getArgList() const { return _argList; }
    std::list<ArgGroup *> getArgGroups() {
        std::list<ArgGroup *> groups = _argGroups;
        groups.push_back(&_autoArgs);
        return groups;
    }

    char getDelimiter() const { return _delimiter; }
    std::string getMessage() const { return _message; }
    bool hasHelpAndVersion() const { return _helpAndVersion; }

    /**
     * Disables or enables CmdLine's internal parsing exception handling.
     *
     * @param state Should CmdLine handle parsing exceptions internally?
     */
    void setExceptionHandling(const bool state);

    /**
     * Returns the current state of the internal exception handling.
     *
     * @retval true Parsing exceptions are handled internally.
     * @retval false Parsing exceptions are propagated to the caller.
     */
    bool hasExceptionHandling() const { return _handleExceptions; }

    /**
     * Allows the CmdLine object to be reused.
     */
    void reset();

    /**
     * Allows unmatched args to be ignored. By default false.
     *
     * @param ignore If true the cmdline will ignore any unmatched args
     * and if false it will behave as normal.
     */
    void ignoreUnmatched(const bool ignore);

    void beginIgnoring() { _ignoring = true; }
    bool ignoreRest() { return _ignoring; }
};

///////////////////////////////////////////////////////////////////////////////
// Begin CmdLine.cpp
///////////////////////////////////////////////////////////////////////////////

inline CmdLine::CmdLine(const std::string &m, char delim, const std::string &v,
                        bool help)
    : _argList(),
      _standaloneArgs(),
      _autoArgs(),
      _argGroups(),
      _progName("not_set_yet"),
      _message(m),
      _version(v),
      _numRequired(0),
      _delimiter(delim),
      _deleteOnExit(),
      _defaultOutput(),
      _output(&_defaultOutput),
      _handleExceptions(true),
      _helpAndVersion(help),
      _ignoreUnmatched(false),
      _ignoring(false) {
    _constructor();
}

inline void CmdLine::_constructor() {
    Arg::setDelimiter(_delimiter);

    Visitor *v;
    CmdLine::add(_standaloneArgs);
    _autoArgs.setParser(*this);
    // add(_autoArgs);

    v = new IgnoreRestVisitor(*this);
    SwitchArg *ignore = new SwitchArg(
        Arg::flagStartString(), Arg::ignoreNameString(),
        "Ignores the rest of the labeled arguments following this flag.", false,
        v);
    _deleteOnExit(ignore);
    _deleteOnExit(v);
    _autoArgs.add(ignore);
    CmdLine::addToArgList(ignore);

    if (_helpAndVersion) {
        v = new HelpVisitor(this, &_output);
        SwitchArg *help = new SwitchArg(
            "h", "help", "Displays usage information and exits.", false, v);
        _deleteOnExit(help);
        _deleteOnExit(v);

        v = new VersionVisitor(this, &_output);
        SwitchArg *vers = new SwitchArg(
            "", "version", "Displays version information and exits.", false, v);
        _deleteOnExit(vers);
        _deleteOnExit(v);

        // A bit of a hack on the order to make tests easier to fix,
        // to be reverted
        _autoArgs.add(vers);
        CmdLine::addToArgList(vers);
        _autoArgs.add(help);
        CmdLine::addToArgList(help);
    }
}

inline void CmdLine::xorAdd(const std::vector<Arg *> &args) {
    OneOf *group = new OneOf(*this);
    _deleteOnExit(group);

    for (std::vector<Arg *>::const_iterator it = args.begin(); it != args.end();
         ++it) {
        group->add(**it);
    }
}

inline void CmdLine::xorAdd(Arg &a, Arg &b) {
    std::vector<Arg *> ors;
    ors.push_back(&a);
    ors.push_back(&b);
    xorAdd(ors);
}

inline ArgContainer &CmdLine::add(ArgGroup &args) {
    args.setParser(*this);
    _argGroups.push_back(&args);

    return *this;
}

inline ArgContainer &CmdLine::add(Arg &a) { return add(&a); }

// TODO: Rename this to something smarter or refactor this logic so
// it's not needed.
inline void CmdLine::addToArgList(Arg *a) {
    for (ArgListIterator it = _argList.begin(); it != _argList.end(); it++)
        if (*a == *(*it))
            throw(SpecificationException(
                "Argument with same flag/name already exists!", a->longID()));

    a->addToList(_argList);

    if (a->isRequired()) _numRequired++;
}

inline ArgContainer &CmdLine::add(Arg *a) {
    addToArgList(a);
    _standaloneArgs.add(a);

    return *this;
}

inline void CmdLine::parse(int argc, const char *const *argv) {
    // this step is necessary so that we have easy access to
    // mutable strings.
    std::vector<std::string> args;
    for (int i = 0; i < argc; i++) args.push_back(argv[i]);

    parse(args);
}

inline void CmdLine::parse(std::vector<std::string> &args) {
    bool shouldExit = false;
    int estat = 0;

    try {
        if (args.empty()) {
            // https://sourceforge.net/p/tclap/bugs/30/
            throw CmdLineParseException(
                "The args vector must not be empty, "
                "the first entry should contain the "
                "program's name.");
        }

        // TODO(macbishop): Maybe store the full name somewhere?
        _progName = basename(args.front());
        args.erase(args.begin());

        int requiredCount = 0;
        std::list<ArgGroup *> missingArgGroups;

        // Check that the right amount of arguments are provided for
        // each ArgGroup, and if there are any required arguments
        // missing, store them for later. Other errors will cause an
        // exception to be thrown and parse will exit early.
        /*
        for (std::list<ArgGroup*>::iterator it = _argGroups.begin();
             it != _argGroups.end(); ++it) {
            bool missingRequired = (*it)->validate(args);
            if (missingRequired) {
                missingArgGroups.push_back(*it);
            }
        }
        */

        for (int i = 0; static_cast<unsigned int>(i) < args.size(); i++) {
            bool matched = false;
            for (ArgListIterator it = _argList.begin(); it != _argList.end();
                 it++) {
                Arg &arg = **it;
                // We check if the argument was already set (e.g., for
                // a Multi-Arg) since then we don't want to count it
                // as required again. This is a hack/workaround to
                // make isRequired() imutable so it can be used to
                // display help correctly (also it's a good idea).
                //
                // TODO: This logic should probably be refactored to
                // remove this logic from here.
                bool alreadySet = arg.isSet();
                bool ignore = arg.isIgnoreable() && ignoreRest();
                if (!ignore && arg.processArg(&i, args)) {
                    requiredCount += (!alreadySet && arg.isRequired()) ? 1 : 0;
                    matched = true;
                    break;
                }
            }

            // checks to see if the argument is an empty combined
            // switch and if so, then we've actually matched it
            if (!matched && _emptyCombined(args[i])) matched = true;

            if (!matched && !ignoreRest() && !_ignoreUnmatched)
                throw(
                    CmdLineParseException("Couldn't find match "
                                          "for argument",
                                          args[i]));
        }

        // Once all arguments have been parsed, check that we don't
        // violate any constraints.
        for (std::list<ArgGroup *>::iterator it = _argGroups.begin();
             it != _argGroups.end(); ++it) {
            bool missingRequired = (*it)->validate();
            if (missingRequired) {
                missingArgGroups.push_back(*it);
            }
        }

        if (requiredCount < _numRequired || !missingArgGroups.empty()) {
            missingArgsException(missingArgGroups);
        }

        if (requiredCount > _numRequired) {
            throw(CmdLineParseException("Too many arguments!"));
        }
    } catch (ArgException &e) {
        // If we're not handling the exceptions, rethrow.
        if (!_handleExceptions) {
            throw;
        }

        try {
            _output->failure(*this, e);
        } catch (ExitException &ee) {
            estat = ee.getExitStatus();
            shouldExit = true;
        }
    } catch (ExitException &ee) {
        // If we're not handling the exceptions, rethrow.
        if (!_handleExceptions) {
            throw;
        }

        estat = ee.getExitStatus();
        shouldExit = true;
    }

    if (shouldExit) exit(estat);
}

inline bool CmdLine::_emptyCombined(const std::string &s) {
    if (s.length() > 0 && s[0] != Arg::flagStartChar()) return false;

    for (int i = 1; static_cast<unsigned int>(i) < s.length(); i++)
        if (s[i] != Arg::blankChar()) return false;

    return true;
}

inline void CmdLine::missingArgsException(
    const std::list<ArgGroup *> &missing) {
    int count = 0;

    std::string missingArgList;
    for (ArgListIterator it = _argList.begin(); it != _argList.end(); it++) {
        if ((*it)->isRequired() && !(*it)->isSet()) {
            missingArgList += (*it)->getName();
            missingArgList += ", ";
            count++;
        }
    }

    for (std::list<ArgGroup *>::const_iterator it = missing.begin();
         it != missing.end(); it++) {
        missingArgList += (*it)->getName();
        missingArgList += ", ";
        count++;
    }

    missingArgList = missingArgList.substr(0, missingArgList.length() - 2);

    std::string msg;
    if (count > 1)
        msg = "Required arguments missing: ";
    else
        msg = "Required argument missing: ";

    msg += missingArgList;

    throw(CmdLineParseException(msg));
}

inline void CmdLine::setOutput(CmdLineOutput *co) { _output = co; }

inline void CmdLine::setExceptionHandling(const bool state) {
    _handleExceptions = state;
}

inline void CmdLine::reset() {
    // TODO: This is no longer correct (or perhaps we don't need "reset")
    for (ArgListIterator it = _argList.begin(); it != _argList.end(); it++)
        (*it)->reset();

    _progName.clear();
}

inline void CmdLine::ignoreUnmatched(const bool ignore) {
    _ignoreUnmatched = ignore;
}

///////////////////////////////////////////////////////////////////////////////
// End CmdLine.cpp
///////////////////////////////////////////////////////////////////////////////

}  // namespace TCLAP

#endif  // TCLAP_CMD_LINE_H
