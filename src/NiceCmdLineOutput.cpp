#include "regex"
#include "NiceCmdLineOutput.h"
#include "platformTools.h"
#include "TablePrinter.h"

using std::string;
using std::vector;
using TCLAP::CmdLineInterface;
using std::cout;
using std::endl;

string getBinaryName() {
	return getBinPath().filename().string();
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

		std::cerr << endl << "For complete usage and help, type `" << getBinaryName() << " --help`" << endl << endl;
	} else {
		usage(cli);
	}
}

void NiceCmdLineOutput::printShortUsage(CmdLineInterface& cli, std::ostream& outStream) const {
	string shortUsage = getBinaryName() + " ";

	// Print XOR arguments
	TCLAP::XorHandler xorHandler = cli.getXorHandler();
	const vector<vector<TCLAP::Arg*>> xorArgGroups = xorHandler.getXorList();
	for (const vector<TCLAP::Arg*>& xorArgGroup : xorArgGroups) {
		shortUsage += " {";
		
		for (auto arg : xorArgGroup) shortUsage += arg->shortID() + "|";
		shortUsage.pop_back();

		shortUsage += '}';
	}

	// Print regular arguments
	std::list<TCLAP::Arg*> argList = cli.getArgList();
	for (auto arg : argList) {
		if (xorHandler.contains(arg)) continue;
		
		shortUsage += " " + arg->shortID();
	}

	outStream << shortUsage << endl;
}

void NiceCmdLineOutput::printLongUsage(CmdLineInterface& cli, std::ostream& outStream) const {
	TablePrinter tablePrinter(&outStream, { 20, 56 });

	// Print XOR arguments
	TCLAP::XorHandler xorHandler = cli.getXorHandler();
	const vector<vector<TCLAP::Arg*>> xorArgGroups = xorHandler.getXorList();
	for (const vector<TCLAP::Arg*>& xorArgGroup : xorArgGroups) {
		for (auto arg : xorArgGroup) {
			if (arg != xorArgGroup[0])
				outStream << "-- or --" << endl;

			tablePrinter.printRow({ arg->longID(), arg->getDescription() });
		}
		outStream << endl;
	}

	// Print regular arguments
	std::list<TCLAP::Arg*> argList = cli.getArgList();
	for (auto arg : argList) {
		if (xorHandler.contains(arg)) continue;

		tablePrinter.printRow({ arg->longID(), arg->getDescription() });
	}
}