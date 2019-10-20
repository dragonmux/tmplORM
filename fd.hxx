#ifndef FD__HXX
#define FD__HXX

#include <stdint.h>
#include <cstddef>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utility>
#include <memory>

#ifdef _MSVC
#define O_NOCTTY _O_BINARY
#endif

#ifdef __GNUC__
#define WARN_UNUSED		__attribute__((warn_unused_result))
#else
#define WARN_UNUSED
#endif

using stat_t = struct stat;

struct fd_t final
{
private:
	int32_t fd;
	mutable bool eof;

public:
	constexpr fd_t() noexcept : fd{-1}, eof{false} { }
	constexpr fd_t(const int32_t fd_) noexcept : fd{fd_}, eof{false} { }
	fd_t(const char *const file, const int flags, const mode_t mode = 0) noexcept :
		fd{::open(file, flags, mode)}, eof{false} { }
	fd_t(fd_t &&fd_) : fd_t{} { swap(fd_); }
	~fd_t() noexcept { if (fd != -1) close(fd); }
	void operator =(fd_t &&fd_) noexcept { swap(fd_); }
	operator int32_t() const noexcept WARN_UNUSED { return fd; }
	bool operator ==(const int32_t desc) const noexcept WARN_UNUSED { return fd == desc; }
	bool valid() const noexcept WARN_UNUSED { return fd != -1; }
	bool isEOF() const noexcept WARN_UNUSED { return eof; }
	void swap(fd_t &desc) noexcept
	{
		std::swap(fd, desc.fd);
		std::swap(eof, desc.eof);
	}

	bool read(void *const value, const size_t valueLen, size_t &resultLen) const noexcept WARN_UNUSED
	{
		if (eof)
			return false;
		const ssize_t result = ::read(fd, value, valueLen);
		if (!result && valueLen)
			eof = true;
		else if (result < 0)
			return false;
		resultLen = size_t(result);
		return resultLen == valueLen;
	}

	bool read(void *const value, const size_t valueLen) const noexcept WARN_UNUSED
	{
		size_t resultLen = 0;
		return read(value, valueLen, resultLen);
	}

	ssize_t read(void *const bufferPtr, const size_t len, std::nullptr_t) const noexcept WARN_UNUSED
		{ return ::read(fd, bufferPtr, len); }
	ssize_t write(const void *const bufferPtr, const size_t valueLen) const noexcept WARN_UNUSED
		{ return ::write(fd, bufferPtr, valueLen); }
	off_t seek(const off_t offset, const int32_t whence) const noexcept WARN_UNUSED
		{ return ::lseek(fd, offset, whence); }
	off_t tell() const noexcept WARN_UNUSED { return seek(0, SEEK_CUR); }
	fd_t dup() const noexcept WARN_UNUSED { return ::dup(fd); }

	stat_t stat() const noexcept WARN_UNUSED
	{
		stat_t fileStat{};
		if (!::fstat(fd, &fileStat))
			return fileStat;
		return {};
	}

	off_t length() const noexcept WARN_UNUSED
	{
		stat_t fileStat{};
		const int result = fstat(fd, &fileStat);
		return result ? -1 : fileStat.st_size;
	}

	template<typename T> bool read(T &value) const noexcept
		{ return read(&value, sizeof(T)); }
	template<typename T, size_t N> bool read(std::array<T, N> &value) const noexcept
		{ return read(value.data(), sizeof(T) * N); }
	template<typename T> bool read(std::unique_ptr<T> &value) const noexcept
		{ return read(value.get(), sizeof(T)); }
	template<typename T> bool read(std::unique_ptr<T []> &value, const size_t valueCount) const noexcept
		{ return read(value.get(), sizeof(T) * valueCount); }

	template<size_t length, typename T, size_t N> bool read(std::array<T, N> &value) const noexcept
	{
		static_assert(length <= N, "Can't request to read more than the std::array<> length");
		return read(value.data(), sizeof(T) * length);
	}

	fd_t(const fd_t &) = delete;
	fd_t &operator =(const fd_t &) = delete;
};

inline void swap(fd_t &a, fd_t &b) noexcept { a.swap(b); }

#endif /*FD__HXX*/
