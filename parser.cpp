#include "parser.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

message parse(const std::string & line) {
	namespace ascii = boost::spirit::ascii;
	using namespace boost::spirit::qi;
	namespace P = boost::phoenix;
	using namespace P;

	typedef std::string::const_iterator it;

	rule<it, std::string()> optprefix;
	optprefix = -(':' >> *(char_ - ' '));

	rule<it, std::string()> argument;
	argument = ((char_ - ' ' - ':') >> *(char_ - ' ')) | (':' >> *char_);

	it first = line.begin();
	it last = line.end();

	message res;

	bool r = phrase_parse(first, last,
		optprefix[P::ref(res.prefix) = _1] >> *(argument[push_back(P::ref(res.args), _1)])
		,
		space);

	return res;
};
// vim:set ts=4 sw=4 sts=4:
