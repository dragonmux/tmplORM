#include "string.hxx"
#include <cstring>
#include <new>

std::unique_ptr<const char []> formatString(const char *format, ...) noexcept
{
	va_list args;
	va_start(args, format);
	auto ret = vaFormatString(format, args);
	va_end(args);
	return ret;
}

std::unique_ptr<const char []> vaFormatString(const char *format, va_list args) noexcept
{
	const size_t len = vsnprintf(NULL, 0, format, args) + 1;
	std::unique_ptr<char []> ret(new (std::nothrow) char[len]);
	if (!ret)
		return nullptr;
	vsprintf(ret.get(), format, args);
	return std::unique_ptr<const char []>(ret.release());
}
