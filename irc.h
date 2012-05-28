#include <stdexcept>
#include <functional>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "parser.h"

namespace asio = boost::asio;
using boost::asio::ip::tcp;
using boost::system::error_code;

struct irc {
	asio::io_service io;
	tcp::resolver resolver;
	tcp::socket socket;
	asio::streambuf requestbuffer;
	asio::streambuf responsebuffer;
	std::stringstream sendbuffer;
	bool ongoingWrite;
	bool sendPending;
	std::ostream & log;
	std::string incompleteline;
	std::string m_nick;

	inline irc()
		: resolver(io)
		, socket(io)
		, ongoingWrite(false)
		, sendPending(false)
		, log(std::cout)
		, m_nick("ravbot")
	{
	}

	inline bool connected() {
		return socket.is_open();
	}

	inline void nick(std::string newnick) {
		m_nick = newnick;
		if (connected()) send_line() << "NICK " << newnick;
	}

	inline void connect(std::string hostname) {
		tcp::resolver::query query(hostname, "6667");
		resolver.async_resolve(query,
			[&] (const error_code & ec, tcp::resolver::iterator iterator) {
				if (ec) {
					log << "Error: " << ec.message() << std::endl;
					return;
				}
				asio::async_connect(socket, iterator,
					[&] (const error_code & ec, tcp::resolver::iterator iterator) {
						if (ec) {
							log << "Connect error: " << ec.message() << std::endl;
							return;
						}
						send_registration();
						async_read();
					}
				);
			}
		);
	}

	inline void run() {
		io.run();
	}

	inline void async_read() {
		asio::async_read_until(socket, responsebuffer, "\r\n",
			boost::bind(&irc::handle_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}

	inline void handle_read(const error_code & ec, std::size_t bytes_transferred) {
		if (ec) {
			log << "!!! Read error: " << ec.message() << std::endl;
			return;
		}
		std::istream is(&responsebuffer);
		std::string line;
		while (getline(is, line)) {
			if (!incompleteline.empty()) {
				line = incompleteline+line;
				incompleteline = "";
			}
			if (line.size() < 1 || line[line.size()-1] != '\r') {
				incompleteline = line;
				continue;
			}
			line.resize(line.size()-1);
			handle_line(line);
		}
		async_read();
	}

	inline void handle_line(const std::string & line) {
		log << "<<< " << line << std::endl;
		message m = parse(line);
		if (!m.args.size()) return;
		if (m.args[0] == "PING")
			send_line() << "PONG " << m.args[1];
		else if (m.args[0] == "INVITE")
			send_line() << "JOIN " << m.args[2];
	}

	inline void send_registration() {
		send_line() << "NICK " << m_nick;
		send_line("USER ravbot ravbot ravbot :Rav bot");
	}

	inline void send_line(std::string line) {
		log << ">>> " << line << std::endl;
		sendbuffer << line << "\r\n";
		flush_send_buffer();
	}

	struct stream_slurper {
		inline stream_slurper(irc & i)
			: i(i)
		{
		}

		inline stream_slurper(const stream_slurper & other)
			: i(other.i)
		{
		}

		template <typename T>
		inline stream_slurper & operator<<(const T & other) {
			ss << other;
			return *this;
		}

		inline ~stream_slurper() {
			std::string line = ss.str();
			if (line.size())
				i.send_line(line);
		}

	private:
		irc & i;
		std::stringstream ss;
	};

	inline stream_slurper send_line() {
		return stream_slurper(*this);
	}

	inline void flush_send_buffer() {
		sendPending = true;
		if (ongoingWrite) return;
		ongoingWrite = true;
		sendPending = false;
		std::ostream request(&requestbuffer);
		request << sendbuffer.str();
		sendbuffer.str("");
		asio::async_write(socket, requestbuffer,
			[&] (const error_code & ec, std::size_t /*bytes_transferred*/) {
				if (ec) log << "Write failed: " << ec.message() << std::endl;
				ongoingWrite = false;
				if (sendPending) flush_send_buffer();
			}
		);
	}

};
// vim:set ts=4 sw=4 sts=4:
