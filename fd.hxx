#ifndef FD__HXX
#define FD__HXX

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utility>

struct fd_t final
{
private:
	int32_t fd;

public:
	constexpr fd_t() noexcept : fd{-1} { }
	constexpr fd_t(const int32_t fd_) noexcept : fd{fd_} { }
	fd_t(const char *const file, const int flags, const mode_t mode = 0) :
		fd{::open(file, flags, mode)} { }
	fd_t(fd_t &&fd_) : fd_t() { swap(fd_); }
	~fd_t() noexcept { if (fd != -1) close(fd); }
	void operator =(fd_t &&fd_) noexcept { swap(fd_); }
	operator int32_t() const noexcept { return fd; }
	bool operator ==(const int32_t fd_) const noexcept { return fd == fd_; }
	void swap(fd_t &fd_) noexcept { std::swap(fd, fd_.fd); }

	ssize_t write(const void *const bufferPtr, const size_t valueLen) const noexcept
		{ return ::write(fd, bufferPtr, valueLen); }

	struct ::stat stat() const noexcept
	{
		struct ::stat fileStat{};
		if (!fstat(fd, &fileStat))
			return fileStat;
		return {};
	}

	bool read(void *const value, const size_t valueLen, size_t &resultLen) const noexcept
	{
		ssize_t ret = ::read(fd, value, valueLen);
		if (ret < 0)
			return false;
		resultLen = size_t(ret);
		return ret || (!valueLen && !ret);
	}

	bool read(void *const value, const size_t valueLen) const noexcept
	{
		size_t resultLen = 0;
		if (!read(value, valueLen, resultLen))
			return false;
		return valueLen == resultLen;
	}

	template<typename T> bool read(T &value) const noexcept
		{ return read(&value, sizeof(T)); }
	template<typename T, size_t N> bool read(std::array<T, N> &value) const noexcept
		{ return read(value.data(), sizeof(T) * N); }

	fd_t(const fd_t &) = delete;
	fd_t &operator =(const fd_t &) = delete;
};

#endif /*FD__HXX*/
