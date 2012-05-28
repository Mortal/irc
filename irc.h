#include <stdexcept>
#include <functional>
#include <sstream>

#include "parser.h"
#include "linewise_socket.h"

struct irc {
	std::ostream & m_log;
	linewise_socket<irc> socket;
	std::string m_nick;

	inline irc()
		: m_log(std::cout)
		, socket(*this, m_log)
		, m_nick("ravbot")
	{
	}

	inline void nick(std::string newnick) {
		m_nick = newnick;
		if (socket.connected()) socket.send_line() << "NICK " << newnick;
	}

	inline void connect(std::string hostname) {
		socket.connect(hostname, "6667");
	}

	inline void run() {
		socket.run();
	}

protected:
	friend class linewise_socket<irc>;

	inline void on_connect() {
		send_registration();
	}

	inline void on_line(const std::string & line) {
		log() << "<<< " << line << std::endl;
		message m = parse(line);
		if (!m.args.size()) return;
		if (m.args[0] == "PING")
			socket.send_line() << "PONG " << m.args[1];
		else if (m.args[0] == "INVITE")
			socket.send_line() << "JOIN " << m.args[2];
		else if (m.args[0] == "PRIVMSG") {
			std::string name(m.prefix.begin(), m.prefix.begin()+m.prefix.find('!'));
			log() << "<" << name << "> " << m.args[2] << std::endl;
		}
	}

private:
	inline std::ostream & log() { return m_log; }

	inline void send_registration() {
		socket.send_line() << "NICK " << m_nick;
		socket.send_line() << "USER ravbot ravbot ravbot :Rav bot";
	}
};
// vim:set ts=4 sw=4 sts=4:
