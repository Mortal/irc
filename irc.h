#include <stdexcept>
#include <functional>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

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

	inline irc()
		: resolver(io)
		, socket(io)
		, ongoingWrite(false)
		, sendPending(false)
		, log(std::cout)
	{
	}

	inline void connect(std::string hostname) {
		tcp::resolver::query query(hostname, "6667");
		resolver.async_resolve(query,
			[&] (const error_code & ec, tcp::resolver::iterator iterator) {
				if (ec) {
					std::cout << "Error: " << ec.message() << std::endl;
					return;
				}
				asio::async_connect(socket, iterator,
					[&] (const error_code & ec, tcp::resolver::iterator iterator) {
						if (ec) {
							std::cout << "Connect error: " << ec.message() << std::endl;
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
		log << "async_read" << std::endl;
		asio::async_read_until(socket, responsebuffer, "\r\n",
			boost::bind(&irc::handle_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}

	inline void handle_read(const error_code & ec, std::size_t bytes_transferred) {
		if (ec) {
			log << "Read error: " << ec.message() << std::endl;
			return;
		}
		std::istream is(&responsebuffer);
		std::string line;
		while (getline(is, line)) {
			if (line.size() < 1) {
				log << "Input line too short" << std::endl;
				continue;
			}
			if (line[line.size()-1] != '\r') {
				log << "Input line terminated incorrectly" << std::endl;
				continue;
			}
			line.resize(line.size()-1);
			handle_line(line);
		}
		log << "done handling read" << std::endl;
		async_read();
	}

	inline void handle_line(const std::string & line) {
		log << "Input line: \"" << line << "\"" << std::endl;
	}

	inline void send_registration() {
		send_line("NICK ravbot");
		send_line("USER ravbot ravbot ravbot :Rav bot");
	}

	inline void send_line(std::string line) {
		log << "Send: " << line << std::endl;
		sendbuffer << line << "\r\n";
		flush_send_buffer();
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
				log << "Flushed send buffer" << std::endl;
				if (ec) log << "Write failed: " << ec.message() << std::endl;
				ongoingWrite = false;
				if (sendPending) flush_send_buffer();
			}
		);
	}
};
// vim:set ts=4 sw=4 sts=4:
