#include "irc.h"

int main(int argc, char ** argv) {
	std::string host = "irc.dsau.dk";
	if (argc > 1) host = argv[1];
	irc instance;
	instance.connect(host);
	instance.run();
	return 0;
}
// vim:set ts=4 sw=4 sts=4:
