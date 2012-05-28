#ifndef __LINEWISE_SOCKET_H__
#define __LINEWISE_SOCKET_H__

#include "stream_slurper.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>

namespace asio = boost::asio;
using boost::asio::ip::tcp;
using boost::system::error_code;

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

#endif // __LINEWISE_SOCKET_H__
// vim:set ts=4 sw=4 sts=4:
