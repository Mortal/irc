#include <stdexcept>
#include <functional>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "parser.h"

namespace asio = boost::asio;
using boost::asio::ip::tcp;
using boost::system::error_code;

template <typename Handler>
struct stream_slurper {
	inline stream_slurper(Handler & i)
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
	Handler & i;
	std::stringstream ss;
};

template <typename Handler>
struct linewise_socket {
	asio::io_service io;
	tcp::resolver resolver;
	tcp::socket socket;
	asio::streambuf requestbuffer;
	asio::streambuf responsebuffer;
	std::stringstream sendbuffer;
	bool ongoingWrite;
	bool sendPending;
	std::ostream & m_log;
	Handler & h;
	std::string incompleteline;

	inline linewise_socket(Handler & h, std::ostream & logger)
		: resolver(io)
		, socket(io)
		, ongoingWrite(false)
		, sendPending(false)
		, m_log(logger)
		, h(h)
	{
	}

	inline bool connected() {
		return socket.is_open();
	}

	inline void connect(std::string hostname, std::string protocol) {
		tcp::resolver::query query(hostname, protocol);
		resolver.async_resolve(query,
			[&] (const error_code & ec, tcp::resolver::iterator iterator) {
				if (ec) {
					log() << "Error: " << ec.message() << std::endl;
					return;
				}
				asio::async_connect(socket, iterator,
					[&] (const error_code & ec, tcp::resolver::iterator iterator) {
						if (ec) {
							log() << "Connect error: " << ec.message() << std::endl;
							return;
						}
						h.on_connect();
						async_read();
					}
				);
			}
		);
	}

	inline void run() {
		io.run();
	}

	inline stream_slurper<linewise_socket> send_line() {
		return stream_slurper<linewise_socket>(*this);
	}

protected:
	friend class stream_slurper<linewise_socket>;

	inline void send_line(std::string line) {
		log() << ">>> " << line << std::endl;
		sendbuffer << line << "\r\n";
		flush_send_buffer();
	}

private:
	inline std::ostream & log() { return m_log; }

	inline void async_read() {
		asio::async_read_until(socket, responsebuffer, "\r\n",
			boost::bind(&linewise_socket::handle_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}

	inline void handle_read(const error_code & ec, std::size_t bytes_transferred) {
		if (ec) {
			log() << "!!! Read error: " << ec.message() << std::endl;
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
			h.on_line(line);
		}
		async_read();
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
				if (ec) log() << "Write failed: " << ec.message() << std::endl;
				ongoingWrite = false;
				if (sendPending) flush_send_buffer();
			}
		);
	}
};

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
	}

private:
	inline std::ostream & log() { return m_log; }

	inline void send_registration() {
		socket.send_line() << "NICK " << m_nick;
		socket.send_line() << "USER ravbot ravbot ravbot :Rav bot";
	}
};
// vim:set ts=4 sw=4 sts=4:
