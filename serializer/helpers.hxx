#ifndef tmplORM_JSON_HELPERS__HXX
#define tmplORM_JSON_HELPERS__HXX

#include <stdint.h>
#include <regex>
#include <rSON.h>

/*!
 * @file
 * @author Rachel Mant
 * @brief Defines helper routines and types for validating decoded JSON input to ORM structures
 */

using jsonAtom_t = rSON::JSONAtom;
using jsonString_t = rSON::JSONString;

inline bool isHex(const char x) noexcept
{
	return (x >= '0' && x <= '9') ||
		(x >= 'A' && x <= 'F') ||
		(x >= 'a' && x <= 'f');
}

inline bool isHex(const char *const str) noexcept
{
	for (size_t i = 0; i < strlen(str); ++i)
	{
		if (!isHex(str[i]))
			return false;
	}
	return true;
}

inline bool isBase64(const char chr) noexcept
{
	return (chr >= '0' && chr <= '9') ||
		(chr >= 'A' && chr <= 'Z') ||
		(chr >= 'a' && chr <= 'z') ||
		chr == '+' || chr == '/';
}

inline bool isBase64(const char *const str, const size_t len) noexcept
{
	if (len & 0x03) // Same as len % 4
		return false;
	for (size_t i = 0; i < len; ++i)
	{
		if (!isBase64(str[i]) && !((i + 3) >= len && (i & 0x03) && str[i] == '='))
			return false;
	}
	return true;
}

inline bool isSlash(const char x) noexcept { return x == '/'; }
inline bool validateID(const int id) noexcept { return id >= 1; }
inline bool validatePort(const int32_t port) noexcept { return port >= 1 && port <= 65535; }
inline bool validateBase64(const jsonString_t &str) { return isBase64(str, str.len()); }
inline bool validateHex(const jsonString_t &hex, const size_t length) noexcept
	{ return hex.len() == length && isHex(hex.get()); }
template<bool func(const jsonString_t &)> bool validateIfString(const jsonAtom_t &node) noexcept
	{ if (node.typeIs(rSON::JSON_TYPE_STRING)) return func(node); return true; }
template<bool func(const int)> bool validateIfInt(const jsonAtom_t &node) noexcept
	{ if (node.typeIs(rSON::JSON_TYPE_INT)) return func(node); return true; }

inline bool validateUUID(const jsonString_t &uuid) noexcept
{
	const auto uuidRegex = std::regex("^[0-9A-Fa-f]{8}-(?:[0-9A-Fa-f]{4}-){3}[0-9A-Fa-f]{12}$");
	return uuid.len() == 36 && std::regex_match(uuid.asString(), uuidRegex);
}

inline bool validateDate(const jsonString_t &date) noexcept
{
	// TODO: This is not technically correct as it allows invalid dates just fine,
	// though at least it correctly checks the form of the date.
	const auto dateRegex = std::regex("^[0-9]{4}-[0-9]{2}-[0-9]{2}$");
	return date.len() == 10 && std::regex_match(date.asString(), dateRegex);
}

inline bool validateTime(const jsonString_t &time) noexcept
{
	// This regex is not technically correct as it allows invalid times just fine,
	// but it does ensure the time conforms to RFC3339 (International Date-Time, aka ISO 8601)
	const auto timeRegex = std::regex("^[0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]+Z$");
	return time.len() >= 11 && std::regex_match(time.asString(), timeRegex);
}

inline bool validateDateTime(const jsonString_t &dateTime) noexcept
{
	// This regex is not technically correct as it allows invalid dates and times just fine,
	// but it does ensure the date-time stamp conforms to RFC3339 (International Date-Time, aka ISO 8601)
	const auto dateTimeRegex = std::regex("^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]+Z$");
	return dateTime.len() >= 22 && std::regex_match(dateTime.asString(), dateTimeRegex);
}

inline bool validateUNCPath(const jsonString_t &path) noexcept
{
	const auto uncRegex = std::regex("^\\\\\\\\[0-9a-zA-Z_-]+(\\\\[0-9a-zA-Z`~!@#$%^&(){}\\[\\]'._-]+)+\\\\?$");
	return std::regex_match(path.asString(), uncRegex);
}

inline bool validateWindowsPath(const jsonString_t &path) noexcept
{
	const auto pathRegex = std::regex("^[A-Z]:(\\\\[^\x00-\x1F<>:\"\\\\/|\\?*]+)+\\\\?$");
	return std::regex_match(path.asString(), pathRegex);
}

inline bool validateUnixPath(const jsonString_t &pathStr) noexcept
{
	bool slash = false, surrogate = false;
	if (pathStr.len() < 1)
		return false;
	// jsonString_t auto-decays to a const char *const
	const char *const path = pathStr;

	for (size_t i = 0; i < pathStr.len(); ++i)
	{
		if (isSlash(path[i]))
		{
			if (slash)
				return false;
			slash = true;
		}
		else
		{
			if (path[i] == char(0xC0))
				surrogate = true;
			// Check for surrogate encoded 0-byte
			else if (surrogate && path[i] == char(0x80))
				return false;
			else
				surrogate = false;
			slash = false;
		}
	}
	return false;
}

inline bool validatePath(const jsonString_t &path) noexcept
	{ return validateUNCPath(path) || validateWindowsPath(path) || validateUnixPath(path); }

#endif /*tmplORM_JSON_HELPERS__HXX*/
