#pragma once

#include <cstddef>
#include <cstdint>

typedef std::uint8_t u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef std::uint64_t u64;

typedef std::int8_t i8;
typedef std::int16_t i16;
typedef std::int32_t i32;
typedef std::int64_t i64;

typedef std::size_t usize;

template <typename T>
struct vec2 {
	T x = T{};
	T y = T{};

	inline T dot(const vec2<T>& v) const {
		return x * v.x + y * v.y;
	}

	inline vec2<T> operator+(const vec2<T>& v) const {
		return { x + v.x, y + v.y };
	}

	inline vec2<T>& operator+=(const vec2<T>& v) {
		x += v.x;
		y += v.y;
		return *this;
	}

	inline vec2<T> operator*(T a) const {
		return { x * a, y * a };
	}

	inline vec2<T>& operator*=(T a) {
		x *= a;
		y *= a;
		return *this;
	}

	friend vec2 operator*(T a, const vec2& v) {
		return { a * v.x, a * v.y };
	}

	void print() const {
		std::cout << "(" << x << ", " << y << ")" << std::endl;
	}
};