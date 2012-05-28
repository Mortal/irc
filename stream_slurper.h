#ifndef __STREAM_SLURPER_H__
#define __STREAM_SLURPER_H__

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

#endif // __STREAM_SLURPER_H__
// vim:set ts=4 sw=4 sts=4:
