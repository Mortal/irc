#ifndef __PARSER_H__
#define __PARSER_H__

#include <vector>
#include <string>

struct message {
	std::string prefix;
	std::vector<std::string> args;
};

message parse(const std::string & line);

#endif // __PARSER_H__
// vim:set ts=4 sw=4 sts=4:
