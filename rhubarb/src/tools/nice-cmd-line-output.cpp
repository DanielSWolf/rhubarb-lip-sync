#include "nice-cmd-line-output.h"

#include <algorithm>

#include "platform-tools.h"
#include "regex"
#include "table-printer.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;
using TCLAP::CmdLineInterface;

string getBinaryName() {
    return getBinPath().filename().u8string();
}

void NiceCmdLineOutput::version(CmdLineInterface& cli) {
    cout << endl << cli.getMessage() << " version " << cli.getVersion() << endl << endl;
}

void NiceCmdLineOutput::usage(CmdLineInterface& cli) {
    cout << endl << "Short usage:" << endl;
    printShortUsage(cli, cout);
    cout << endl;

    cout << "Long usage:" << endl << endl;
    printLongUsage(cli, cout);

    cout << endl;
}

void NiceCmdLineOutput::failure(CmdLineInterface& cli, TCLAP::ArgException& e) {
    std::cerr << "Invalid command-line arguments. " << e.argId() << endl;
    std::cerr << e.error() << endl << endl;

    if (cli.hasHelpAndVersion()) {
        std::cerr << "Short usage:" << endl;
        printShortUsage(cli, std::cerr);

        std::cerr
            << endl
            << "For complete usage and help, type `"
            << getBinaryName()
            << " --help`"
            << endl
            << endl;
    } else {
        usage(cli);
    }
}

// Prints regular arguments, ignoring argument groups (which we don't use anyway)
void NiceCmdLineOutput::printShortUsage(CmdLineInterface& cli, std::ostream& outStream) const {
    string shortUsage = getBinaryName() + " ";
    for (auto arg : cli.getArgList())
        shortUsage += " " + arg->shortID();

    outStream << shortUsage << endl;
}

// Printa regular arguments, ignoring argument groups (which we don't use anyway)
void NiceCmdLineOutput::printLongUsage(CmdLineInterface& cli, std::ostream& outStream) const {
    TablePrinter tablePrinter(&outStream, {20, 56});
    for (auto arg : cli.getArgList())
        tablePrinter.printRow({arg->longID(), arg->getDescription()});
}
