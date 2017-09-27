#pragma once

#include "Formatter.h"

namespace logging {

	class SimpleConsoleFormatter : public Formatter {
	public:
		std::string format(const Entry& entry) override;
	};

	class SimpleFileFormatter : public Formatter {
	public:
		std::string format(const Entry& entry) override;
	private:
		SimpleConsoleFormatter consoleFormatter;
	};

}
