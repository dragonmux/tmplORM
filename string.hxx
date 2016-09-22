#ifndef STRING__HXX
#define STRING__HXX

#include <memory>
#include <cstdarg>

extern std::unique_ptr<const char []> formatString(const char *format, ...) noexcept;
extern std::unique_ptr<const char []> vaFormatString(const char *format, va_list args) noexcept;

#endif /*STRING__HXX*/
