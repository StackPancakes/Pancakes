#ifndef IO_HPP
#define IO_HPP

#include <cstdio>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace io
{
	inline void write(char const* data, std::size_t size) noexcept
	{
		std::fwrite(data, 1, size, stdout);
	}

	inline void write(std::string_view sv) noexcept
	{
		write(sv.data(), sv.size());
	}

	inline void flush() noexcept
	{
		std::fflush(stdout);
	}

	inline void print(std::string_view sv) noexcept
	{
		write(sv);
	}

	inline void println(std::string_view sv = "") noexcept
	{
		write(sv);
		write("\n", 1);
	}

	inline void print_line(std::string_view sv = "") noexcept
	{
		println(sv);
	}

	inline void put_char(char c) noexcept
	{
		write(&c, 1);
	}

	inline void put_newline() noexcept
	{
		write("\n", 1);
	}

	template <typename... Args>
	void print(char const* format, Args... args) noexcept
	{
	}

	namespace err 
	{
		inline void write(const char* data, std::size_t size) noexcept 
		{
			std::fwrite(data, 1, size, stderr);
		}

		inline void write(std::string_view sv) noexcept 
		{
			write(sv.data(), sv.size());
		}

		inline void print(std::string_view sv) noexcept 
		{
			write(sv);
		}

		inline void print_line(std::string_view sv = "") noexcept 
		{
			write(sv);
			write("\n", 1);
		}
	}
}

#endif