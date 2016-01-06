#ifndef RHUBARB_LIP_SYNC_NICECMDLINEOUTPUT_H
#define RHUBARB_LIP_SYNC_NICECMDLINEOUTPUT_H

#include <tclap/StdOutput.h>

class NiceCmdLineOutput : public TCLAP::CmdLineOutput {
public:
	void usage(TCLAP::CmdLineInterface& cli) override;
	void version(TCLAP::CmdLineInterface& cli) override;
	void failure(TCLAP::CmdLineInterface& cli, TCLAP::ArgException& e) override;
private:
	// Writes a brief usage message with short args.
	void printShortUsage(TCLAP::CmdLineInterface& cli, std::ostream& outStream) const;

	// Writes a longer usage message with long and short args, providing descriptions
	void printLongUsage(TCLAP::CmdLineInterface& cli, std::ostream& outStream) const;
};

#endif //RHUBARB_LIP_SYNC_NICECMDLINEOUTPUT_H
