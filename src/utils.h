#pragma once

#include <chrono>
#include <iomanip>
#include <sstream>

template <typename F>
struct privDefer {
	F f;
	privDefer(F f) : f(f) {}
	~privDefer() { f(); }
};

template <typename F>
privDefer<F> defer_func(F f) {
	return privDefer<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer(code)   auto DEFER_3(_defer_) = defer_func([&](){code;})

inline std::string to_hex(Sha256 byte_array) {
	std::ostringstream os;
	os << "0x" << std::hex;

	for (Byte byte : byte_array) {
		os << std::setw(2) << std::setfill('0') << (int)byte;
	}

	return os.str();
}
