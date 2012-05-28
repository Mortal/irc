#include <algorithm>
#include <iostream>
#include "parser.h"
using namespace std;
int main() {
	string line;
	while (getline(cin, line)) {
		message m = parse(line);
		cout << "Prefix: " << m.prefix << "\nArgs:\n";
		for_each(m.args.begin(), m.args.end(), [] (string s) {
			cout << s << endl;
		});
		cout << endl;
	}
	return 0;
}
// vim:set ts=4 sw=4 sts=4:
