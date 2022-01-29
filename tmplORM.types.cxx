#include <cstdint>
#include <cerrno>
#include <cstring>
#include <array>
#include <chrono>
#include <substrate/fd>
#include <substrate/utility>
#include <tmplORM.types.hxx>
#include <dateTime.hxx>

using substrate::promoted_type_t;
using substrate::fd_t;

using tmplORM::types::baseTypes::ormDateTime_t;
using tmplORM::types::baseTypes::chrono::durationIn;
using tmplORM::types::baseTypes::chrono::durationAs;
using tmplORM::types::baseTypes::chrono::years;
using tmplORM::types::baseTypes::chrono::days;
using tmplORM::types::baseTypes::chrono::hours;
using tmplORM::types::baseTypes::chrono::seconds;
using tmplORM::types::baseTypes::chrono::operator ""_y;
using tmplORM::types::baseTypes::chrono::operator ""_day;
using tmplORM::types::baseTypes::chrono::isLeap;
using tmplORM::types::baseTypes::chrono::monthDays;

constexpr hours operator ""_h(const unsigned long long value) noexcept { return hours{value}; }
constexpr seconds operator ""_sec(const unsigned long long value) noexcept { return seconds{value}; }

std::array<std::array<uint16_t, 12>, 2> tmplORM::types::baseTypes::chrono::monthDays
{{
	{{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}},
	{{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}}
}};

constexpr static std::array<char, 4> tzMagic{{'T', 'Z', 'i', 'f'}};
struct tzHead_t
{
	std::array<char, 4> magic;
	uint8_t version;
	std::array<char, 15> reserved;
	uint32_t ttIsGmtCount;
	uint32_t ttIsStdCount;
	uint32_t leapCount;
	uint32_t timeCount;
	uint32_t typeCount;
	uint32_t charCount;
};

struct tzString_t
{
	size_t length;
	std::unique_ptr<char []> data;
	std::unique_ptr<tzString_t> next;
};

enum class tzRuleType_t : uint8_t
	{ J0, J1, M };

struct tzRule_t
{
	const char *name;
	tzRuleType_t type;
	uint16_t month;
	uint16_t week;
	uint16_t day;
	int64_t seconds;
	int32_t offset;
	time_t change;
	int32_t yearFor;
};

std::array<const char *, 2> tzName{};
static int32_t ruleStdOffset{};
static int32_t ruleDstOffset{};
static bool isDaylight;
static int32_t timeZone;
std::array<tzRule_t, 2> tzRules{};
bool tzInitialised{false};

static size_t transitionsCount{}, typesCount{}, leapsCount{};
std::unique_ptr<time_t []> transitions{};
std::unique_ptr<uint8_t []> typeIndexes{};
std::unique_ptr<ttInfo_t []> types{};
std::unique_ptr<char []> zoneNames{};
std::unique_ptr<leap_t []> leaps{};
std::unique_ptr<char []> tzSpec{};
static std::unique_ptr<tzString_t> tzStringList{};

// TODO: fixme.
#define TZDIR "/usr/share/zoneinfo"
#define TZDEFAULT "/etc/localtime"
constexpr size_t halfYear = durationIn<seconds>(1_y) / 2;

template<typename T> constexpr size_t signBit() noexcept
	{ return std::numeric_limits<typename std::make_signed<T>::type>::digits; }
constexpr size_t sizeMax = std::numeric_limits<size_t>::max();
constexpr uint8_t sizeBits = std::numeric_limits<size_t>::digits - 1;

inline int32_t asInt32(const uint8_t *const value) noexcept
{
	uint32_t result{};
	if (sizeof(int32_t) != 4 && value[0] >> signBit<char>())
		result = uint32_t(-1) & ~0xFFFFFFFFU;
	result |= uint32_t(value[0] << 24U) | uint32_t(value[1] << 16U) |
		uint32_t(value[2] << 8U) | value[3];
	return static_cast<int32_t>(result);
}
inline int32_t asInt32(const std::array<uint8_t, 4> &value) noexcept
	{ return asInt32(value.data()); }

inline int64_t asInt64(const uint8_t *const value) noexcept
{
	uint64_t result{};
	if (sizeof(int64_t) != 8 && value[0] >> signBit<char>())
		result = uint64_t(-1) & ~0xFFFFFFFFFFFFFFFFU;
	result |= (uint64_t(value[0]) << 56U) | (uint64_t(value[1]) << 48U) |
		(uint64_t(value[2]) << 40U) | (uint64_t(value[3]) << 32U) |
		(uint64_t(value[4]) << 24U) | (uint64_t(value[5]) << 16U) |
		(uint64_t(value[6]) << 8U) | uint64_t(value[7]);
	return static_cast<int64_t>(result);
}

inline int64_t asInt64(const uint8_t *const value, const size_t width) noexcept
{
	if (width == 8)
		return asInt64(value);
	else
		return asInt32(value);
}

size_t safeMul(const size_t a, const size_t b) noexcept
{
	// If we have an error value on the left, return our error value.
	if (a == sizeMax || b == sizeMax)
		return sizeMax;
	// This could also use a mul-only method that computes part of the Karatsuba decomposition
	// and determines from the halved-size result if the multiplied value sets bits above the
	// max for the type - we only need to know z2. eg, for sizeof(size_t) == 8:
	// size_t c = (a >> 32) * (b >> 32); if (c) return sizeMax
	else if (a && (sizeMax / a) < b)
		return sizeMax;
	return a * b;
}

size_t safeAdd(const size_t a, const size_t b) noexcept
{
	// If we have an error value on the left, return our error value.
	if (a == sizeMax || b == sizeMax)
		return sizeMax;
	// Do the top-end addition, and if it produces a value that exceeds 2 bits of storage..
	// overflow would occur and we return sizeMax
	else if ((a >> uint8_t(sizeBits - 1U)) + (b >> uint8_t(sizeBits - 1U)) > 3)
		return sizeMax;
	return a + b;
}

template<typename ...values_t> size_t safeAdd(const size_t a, const size_t b, values_t &&...values) noexcept
	{ return safeAdd(safeAdd(a, b), values...); }

size_t safeSub(const size_t a, const size_t b) noexcept
{
	// If we have an error value on the left, return our error value.
	if (a == sizeMax || b == sizeMax || a < b)
		return sizeMax;
	return a - b;
}

template<typename ...values_t> size_t safeSub(const size_t a, const size_t b, values_t &&...values) noexcept
	{ return safeSub(safeSub(a, b), values...); }

size_t safeAnd(const size_t a, const size_t b) noexcept
{
	// If we have an error value on the left, return our error value.
	if (a == sizeMax || b == sizeMax)
		return sizeMax;
	return a & b;
}

inline bool badRead() noexcept
{
	transitions = nullptr;
	transitionsCount = 0;
	typeIndexes = nullptr;
	types = nullptr;
	typesCount = 0;
	zoneNames = nullptr;
	leaps = nullptr;
	leapsCount = 0;
	return false;
}

fd_t tzOpenFile(const char *const file) noexcept
{
	if (file[0] != '/')
	{
		const char *dir = getenv("TZDIR");
		if (!dir || !dir[0])
			dir = TZDIR;
		auto fileName = formatString("%s/%s", dir, file);
		if (!fileName)
			return {};
		return {fileName.get(), O_RDONLY | O_CLOEXEC};
	}
	return {file, O_RDONLY | O_CLOEXEC};
}

bool readTransitions(const fd_t &fd, const size_t width) noexcept
{
	const auto buffer = substrate::make_unique_nothrow<uint8_t []>(transitionsCount * width);
	if (!buffer ||
		!fd.read(buffer.get(), transitionsCount * width) ||
		!fd.read(typeIndexes, transitionsCount))
		return false;

	if (sizeof(time_t) == 8)
	{
		for (size_t i{transitionsCount}; i > 0; )
		{
			const size_t index = --i;
			transitions[index] = asInt64(buffer.get() + (i * width), width);
			if (typeIndexes[i] >= typesCount)
				return false;
		}
	}
	else if (width == 4)
	{
		for (size_t i{transitionsCount}; i > 0; )
		{
			size_t index = --i;
			transitions[index] = asInt32(buffer.get() + (i * width));
			if (typeIndexes[i] >= typesCount)
				return false;
		}
	}
	return true;
}

bool readTypes(const fd_t &fd, const size_t charCount) noexcept
{
	for (size_t i{}; i < typesCount; ++i)
	{
		std::array<uint8_t, 4> offset{};
		uint8_t value{};

		if (!fd.read(offset) || !fd.read(value) || value > 1)
			return false;
		types[i].isDst = value;
		if (!fd.read(value) || value > charCount)
			return false;
		types[i].index = value;
		types[i].offset = asInt32(offset);
	}
	return true;
}

bool readLeaps(const fd_t &fd, const size_t width) noexcept
{
	uint8_t buffer[8];
	for (size_t i{}; i < leapsCount; ++i)
	{
		if (!fd.read(buffer, width) ||
			!fd.readBE(leaps[i].change))
			return false;
		else if (sizeof(time_t) == 8)
			leaps[i].transition = asInt64(buffer, width);
		else
			leaps[i].transition = asInt32(buffer);
	}
	return true;
}

bool readIsStd(const fd_t &fd, const size_t isStdCount) noexcept
{
	for (size_t i{}; i < typesCount; ++i)
	{
		int8_t value{};
		if (i < isStdCount && !fd.read(value))
			return false;
		types[i].isStd = bool(value);
	}
	return true;
}

bool readIsGmt(const fd_t &fd, const size_t isGmtCount) noexcept
{
	for (size_t i{}; i < typesCount; ++i)
	{
		int8_t value{};
		if (i < isGmtCount && !fd.read(value))
			return false;
		types[i].isGmt = bool(value);
	}
	return true;
}

void readTzSpec(const fd_t &fd, const size_t tzSpecLen, const size_t version) noexcept
{
	if (sizeof(time_t) == 8 && tzSpecLen)
	{
		char junk;
		if (!fd.read(junk) || junk != '\n' ||
			!fd.read(tzSpec, tzSpecLen - 1))
			tzSpec = nullptr;
		else
			tzSpec[tzSpecLen - 1] = 0;
	}
	else if (sizeof(time_t) == 4 && version)
	{
		/*tzHead_t header;
		if (!fd.read(header) || header.magic != tzMagic)
			return badRead();*/
		puts("This mode is not implemented");
	}
	if (tzSpec && !tzSpec[0])
		tzSpec = nullptr;
}

char *tzString(const char *string, const size_t length) noexcept
{
	tzString_t *last{};
	for (tzString_t *value = tzStringList.get(); value; value = value->next.get())
	{
		last = value;
		if (length <= value->length)
		{
			char *const data = &value->data[value->length - length];
			if (memcmp(string, data, length) == 0)
				return data;
		}
	}

	auto value = substrate::make_unique_nothrow<tzString_t>();
	if (!value)
		return nullptr;
	value->data = substrate::make_unique_nothrow<char []>(length + 1);
	if (!value->data)
		return nullptr;
	char *const result = value->data.get();
	memcpy(result, string, length);
	result[length] = 0;
	value->length = length;
	value->next = nullptr;
	if (last)
		last->next = std::move(value);
	else
		tzStringList = std::move(value);
	return result;
}
char *tzString(const char *string) noexcept { return tzString(string, strlen(string)); }
char *tzString(const std::unique_ptr<char []> &string) noexcept { return tzString(string.get()); }

bool registerZones(const size_t charCount) noexcept
{
	zoneNames[charCount] = 0;
	for (size_t i{}; i < typesCount; ++i)
		if (!tzString(&zoneNames[types[i].index]))
			return false;
	return true;
}

bool findZoneNames() noexcept
{
	tzName[0] = nullptr;
	tzName[1] = nullptr;
	for (size_t i{transitionsCount}; i > 0; )
	{
		const uint8_t type = typeIndexes[--i];
		const bool isDst = types[type].isDst;

		if (!tzName[isDst])
		{
			tzName[isDst] = tzString(&zoneNames[types[type].index]);
			if (tzName[!isDst])
				break;
		}
	}
	if (!tzName[0])
	{
		if (typesCount != 1)
			return false;
		tzName[0] = tzString(zoneNames.get());
		if (!tzName[0])
			return false;
	}
	if (!tzName[1])
		tzName[1] = tzName[0];
	return true;
}

void computeRules() noexcept
{
	if (!transitionsCount)
		ruleStdOffset = ruleDstOffset = types[0].offset;
	else
	{
		bool stdSet{false}, dstSet{false};
		ruleStdOffset = 0;
		ruleDstOffset = 0;
		for (size_t i{transitionsCount}; i > 0;)
		{
			--i;
			if (!stdSet && !types[typeIndexes[i]].isDst)
			{
				ruleStdOffset = types[typeIndexes[i]].offset;
				stdSet = true;
			}
			else if (!dstSet && types[typeIndexes[i]].isDst)
			{
				ruleDstOffset = types[typeIndexes[i]].offset;
				dstSet = true;
			}
			if (stdSet && dstSet)
				break;
		}
		if (!dstSet)
			ruleDstOffset = ruleStdOffset;
	}
	isDaylight = ruleStdOffset != ruleDstOffset;
	timeZone = -ruleStdOffset;
}

bool tzReadFile(const char *const file) noexcept
{
	uint8_t width = 4;

	static_assert(sizeof(time_t) == 4 || sizeof(time_t) == 8, "time_t not a valid size");
	fd_t fd{tzOpenFile(file)};
	if (!fd.valid())
		return false;
	const auto fileStat = fd.stat();
	if (fileStat.st_size == 0)
		return false;
	// This is normally used to reset state after an error, however
	// this is not to deal with an error but to ensure good state when entering this function
	badRead();

	size_t charCount{0}, isStdCount{0}, isGmtCount{0};
	uint8_t version{0};
	auto readHeader = [&]() noexcept -> bool
	{
		tzHead_t header{};
		if (!fd.read(header.magic) ||
			header.magic != tzMagic ||
			!fd.read(header.version) ||
			!fd.read(header.reserved) ||
			!fd.readBE(header.ttIsGmtCount) ||
			!fd.readBE(header.ttIsStdCount) ||
			!fd.readBE(header.leapCount) ||
			!fd.readBE(header.timeCount) ||
			!fd.readBE(header.charCount))
			return false;
		transitionsCount = header.timeCount;
		typesCount = header.typeCount;
		charCount = header.charCount;
		leapsCount = header.leapCount;
		isStdCount = header.ttIsStdCount;
		isGmtCount = header.ttIsGmtCount;
		if (isStdCount > typesCount || isGmtCount > typesCount)
			return false;
		if (header.version)
			version = static_cast<uint8_t>(header.version - '0');
		return true;
	};

	if (!readHeader())
		return badRead();
	else if (sizeof(time_t) == 8 && version)
	{
		const auto toSkip{safeAdd(safeMul(transitionsCount, 5U), safeMul(typesCount, 6U),
			charCount, safeMul(leapsCount, 8U), isStdCount, isGmtCount)};
		if (toSkip == sizeMax || !fd.seek(static_cast<off_t>(toSkip), SEEK_CUR) || !readHeader())
			return false;
		width = 8U;
	}
	else if (sizeof(time_t) == 4 && width == 8)
		return false;

	const auto totalSize{safeAdd(safeMul(transitionsCount, width + 1U), safeMul(typesCount, 6U),
		charCount, safeMul(leapsCount, 8U), isStdCount, isGmtCount)};
	if (totalSize == sizeMax)
		return false;
	size_t tzSpecLen{};
	if (sizeof(time_t) == 8 && width == 8)
	{
		const off_t rem = fileStat.st_size - fd.tell();
		if (rem < 0)
			return false;
		tzSpecLen = safeSub(static_cast<size_t>(rem), totalSize, 1U);
		if (tzSpecLen == 0 || tzSpecLen == sizeMax)
			return false;
	}

	transitions = substrate::make_unique_nothrow<time_t []>(transitionsCount);
	typeIndexes = substrate::make_unique_nothrow<uint8_t []>(transitionsCount);
	types = substrate::make_unique_nothrow<ttInfo_t []>(typesCount);
	zoneNames = substrate::make_unique_nothrow<char []>(charCount + 1);
	leaps = leapsCount ? substrate::make_unique_nothrow<leap_t []>(leapsCount) : nullptr;
	tzSpec = tzSpecLen ? substrate::make_unique_nothrow<char []>(tzSpecLen) : nullptr;
	if (!transitions || !typeIndexes || !types ||
		!zoneNames || (leapsCount && !leaps) || (tzSpecLen && !tzSpec) ||
		!readTransitions(fd, width) ||
		!readTypes(fd, charCount) ||
		!fd.read(zoneNames, charCount) ||
		!readLeaps(fd, width) ||
		!readIsStd(fd, isStdCount) ||
		!readIsGmt(fd, isGmtCount))
		return badRead();

	readTzSpec(fd, tzSpecLen, version);
	if (!registerZones(charCount) ||
		!findZoneNames())
		return badRead();
	computeRules();
	return tzInitialised = true;
}

inline bool isAlpha(const char x) noexcept
{
	return
		(x >= 'a' && x <= 'z') ||
		(x >= 'A' && x <= 'Z');
}

inline bool isNumber(const char x) noexcept
	{ return x >= '0' && x <= '9'; }
inline bool isAlphaNum(const char x) noexcept
	{ return isAlpha(x) || isNumber(x); }
inline bool isSign(const char x) noexcept
	{ return x == '+' || x == '-'; }

bool tzParseName(const char *&spec, const uint8_t rule) noexcept
{
	const char *begin = spec, *end = spec;
	while (isAlpha(*end))
		++end;
	auto length = static_cast<size_t>(end - begin);
	if (length < 3U)
	{
		end = spec;
		if (*end++ != '<')
			return false;
		begin = end;
		while (isAlphaNum(*end) || isSign(*end))
			++end;
		length = static_cast<size_t>(end - begin);
		if (*end++ != '>' || length < 3U)
			return false;
	}
	const auto name = tzString(begin, length);
	if (!name)
		return false;
	tzRules[rule].name = name;
	spec = end;
	return true;
}

inline uint16_t tzParseInt(const char *&str) noexcept
{
	using uint_t = promoted_type_t<uint16_t>;
	uint_t value = 0;
	while (isNumber(*str))
	{
		value *= 10;
		value += uint_t(*str++ - '0');
	}
	return static_cast<uint16_t>(value);
}

inline bool tzDefaultRules(const uint8_t rule) noexcept
{
	if (!rule)
		tzRules[0].offset = 0;
	else
		// DST defaults to an offset one hour later than STD.
		tzRules[1].offset = static_cast<int32_t>((seconds{tzRules[0].offset} + 1_h).count());
	return false;
}

inline int64_t computeOffset(uint16_t hours, uint16_t minutes, uint16_t seconds) noexcept
{
	if (hours > 24)
		hours = 24;
	if (minutes > 59)
		minutes = 59;
	if (seconds > 59)
		seconds = 59;
	return (hours * 60 + minutes) * 60 + seconds;
}

bool tzParseTripple(const char *&str, const char seperator, uint16_t &first,
	uint16_t &second, uint16_t &third) noexcept
{
	const char *begin = str;
	const char *end = str;
	first = tzParseInt(end);
	if (begin == end)
		return false;
	else if (*end != seperator)
	{
		str = end;
		return true;
	}
	begin = ++end;
	second = tzParseInt(end);
	if (begin == end)
		return false;
	else if (*end != seperator)
	{
		str = end;
		return true;
	}
	begin = ++end;
	third = tzParseInt(end);
	if (begin == end)
		return false;
	str = end;
	return true;
}

bool tzParseOffset(const char *&spec, const uint8_t rule) noexcept
{
	if (rule == 0 && !(*spec && (isSign(*spec) || isNumber(*spec))))
		return false;
	// If the offset is into the past, then we want to add it, otherwise we want to subtract it.
	const int8_t sign = isSign(*spec) && *spec++ == '-' ? 1 : -1;
	uint16_t hours{}, minutes{}, seconds{};
	if (!tzParseTripple(spec, ':', hours, minutes, seconds))
		return tzDefaultRules(rule);
	tzRules[rule].offset = int32_t(sign * computeOffset(hours, minutes, seconds));
	return true;
}

bool tzParseRule(const char *&spec, const uint8_t ruleIndex) noexcept
{
	const char *start = spec;
	tzRule_t &rule = tzRules[ruleIndex];
	// Skip past any incorrectly-specified POSIX.1 comma
	start += *start == ',';
	if (*start == 'J' || isNumber(*start))
	{
		rule.type = *start == 'J' ? tzRuleType_t::J1 : tzRuleType_t::J0;
		if (rule.type == tzRuleType_t::J1 && !isNumber(*++start))
			return false;
		const char *end = start;
		const auto day = tzParseInt(end);
		if (start == end || day > 365)
			return false;
		else if (rule.type == tzRuleType_t::J1 && day == 0)
			return false;
		rule.day = day;
		start = end;
	}
	else if (*start == 'M')
	{
		rule.type = tzRuleType_t::M;
		++start;
		if (!tzParseTripple(start, '.', rule.month, rule.week, rule.day) ||
			rule.month < 1 || rule.month > 12 ||
			rule.week < 1 || rule.week > 6 ||
			rule.day > 6)
			return false;
	}
	else if (*start == 0)
	{
		/*
		 * Daylight time rules in the US are defined in the document "US Code,
		 * Title 15, Chapter 6, Subchapter IX - Standard Time". These dates were
		 * established by Congress in the Energy Policy Act of 2005 (Public Library
		 * number 109-58, 119 Stat 594 from 2005). Below is the equivilent of the
		 * tzSpec "M3.2.0,M11.1.0" with /2 not needed since 2:00am is the default.
		 */
		rule.type = tzRuleType_t::M;
		if (!ruleIndex)
		{
			rule.month = 3;
			rule.week = 2;
			rule.day = 0;
		}
		else
		{
			rule.month = 11;
			rule.week = 1;
			rule.day = 0;
		}

	}
	else
		return false;

	if (*start && *start != '/' && *start != ',')
		return false;
	else if (*start == '/')
	{
		if (!*++start)
			return false;
		bool negative = *start == '-';
		start += negative;
		uint16_t hours{}, minutes{}, seconds{};
		if (!tzParseTripple(start, ':', hours, minutes, seconds))
		{
			hours = 2;
			minutes = 0;
			seconds = 0;
		}
		rule.seconds = (negative ? -1 : 1) * computeOffset(hours, minutes, seconds);
	}
	else
		rule.seconds = durationIn<seconds>(2_h);

	rule.yearFor = -1;
	spec = start;
	return true;
}

void tzUpdateRules() noexcept
{
	isDaylight = tzRules[0].offset != tzRules[1].offset;
	timezone = -tzRules[0].offset;
	tzName[0] = tzRules[0].name;
	tzName[1] = tzRules[1].name;
}

bool tzParseSpec(const char *&spec, const uint8_t rule) noexcept
	{ return tzParseName(spec, rule) && (tzParseOffset(spec, rule) || rule); }

void tzParseSpec() noexcept
{
	const char *spec = tzSpec.get();
	if (tzParseSpec(spec, 0))
	{
		if (*spec)
		{
			if (tzParseSpec(spec, 1))
			{
				if (!*spec || (*spec++ == ',' && !*spec--))
				{
					// tzReadDefault();
					puts("Mode not currently supported");
				}
			}
			// Now figure out the STD <-> DST rules.
			if (tzParseRule(spec, 0))
				tzParseRule(spec, 1);
		}
		else
		{
			// No DST? No problem. Just make it the same as STD.
			tzRules[1].name = tzRules[0].name;
			tzRules[1].offset = tzRules[0].offset;
		}
	}
	tzUpdateRules();
}

void tzInit() noexcept
{
	const char *tz = getenv("TZ");
	if (!tz)
		tz = TZDEFAULT;
	else if (!tz[0])
		tz = "Universal";
	else if (tz[0] == ':')
		++tz;

	if (tzReadFile(tz))
		return;
	isDaylight = false;
	timeZone = 0;
	tzName[0] = tzString("UTC");
	tzName[1] = tzName[0];
	tzInitialised = true;
}

size_t searchFor(const time_t time) noexcept
{
	size_t low{0};
	size_t high{transitionsCount - 1U};
	size_t i{size_t(transitions[high] - time) / halfYear};
	// If we have a reasonable guess for i, tune low and high to reduce the search space if possible.
	if (i < transitionsCount)
	{
		i = transitionsCount - i - 1;
		if (time < transitions[i])
		{
			// Although, if we're within 10 entries anyway.. just do a naive search
			if (i < 10 || time >= transitions[i - 10])
			{
				while (time < transitions[--i])
					continue;
				return i;
			}
			high = i - 10;
		}
		else
		{
			// Although, if we're within 10 entries anyway.. just do a naive search
			if (i + 10 >= transitionsCount || time < transitions[i + 10])
			{
				while (time >= transitions[i])
					++i;
				return i;
			}
			low = i + 10;
		}
	}

	while (low + 1 < high)
	{
		i = (low + high) / 2;
		if (time < transitions[i])
			high = i;
		else
			low = i;
	}
	return low;
}

uint8_t computeRules(const size_t i) noexcept
{
	const uint8_t type = typeIndexes[i];
	tzName[types[type].isDst] = tzString(&zoneNames[types[type].index]);
	for (size_t j{i + 1}; j < transitionsCount; ++j)
	{
		uint8_t nextType = typeIndexes[j];
		const bool isDst = types[nextType].isDst;
		if (!tzName[isDst])
		{
			tzName[isDst] = tzString(&zoneNames[types[nextType].index]);
			if (tzName[!isDst])
				break;
		}
	}
	if (!tzName[0])
		tzName[0] = tzName[1];
	return type;
}

int32_t computeOffset(const size_t index) noexcept
{
	if (!typesCount)
		return 0;
	const auto &info = types[index];
	isDaylight = ruleStdOffset != ruleDstOffset;
	timezone = -ruleStdOffset;
	if (!tzName[0])
		tzName[0] = tzString(zoneNames);
	if (!tzName[1])
		tzName[1] = tzName[0];
	return info.offset;
}

constexpr int16_t y1970_4 = 1970 / 4;
constexpr int16_t y1970_100 = 1970 / 100;
constexpr int16_t y1970_400 = 1970 / 400;

void computeChange(tzRule_t &rule, const int16_t year) noexcept
{
	time_t time = 0;
	if (year != -1 && rule.yearFor == year)
		return; // We already did the legwork (except for 2BC) cool.. fast path!
	else if (year > 1970)
	{
		const auto _year{static_cast<uint16_t>(year - 1)};
		time = durationIn<seconds>(years{uint32_t(year) - 1970U} + days{(_year / 4U - y1970_4) -
			(_year / 100U - y1970_100) + (_year / 400U - y1970_400)});
	}

	switch (rule.type)
	{
	case tzRuleType_t::J1:
		time += durationIn<seconds>(days{rule.day - 1U});
		if (rule.day >= 60U && isLeap(year))
			time += durationIn<seconds>(1_day);
		break;
	case tzRuleType_t::J0:
		time += durationIn<seconds>(days{rule.day});
		break;
	case tzRuleType_t::M:
		const auto _month{rule.month - 1U};
		const auto &daysFor{monthDays[isLeap(year)]};
		for (uint16_t i = 0; i < _month; ++i)
			time += durationIn<seconds>(days{daysFor[i]});
		const auto month{rule.month < 3U ? rule.month + 12U : rule.month};
		const auto _year{rule.month < 3U ? year - 1 : year};
		const auto _century{std::div(_year, 100)};
		const auto century{_century.quot};
		const auto centuryYear{static_cast<uint8_t>(_century.rem)};
		// We're after the first day of the month here, so we set q from the Gregorian formula
		// on https://en.wikipedia.org/wiki/Zeller%27s_congruence to 0
		const uint32_t dow = uint32_t(13U * (month + 1U) / 5U + centuryYear +
			centuryYear / 4U + uint32_t(century / 4 - 2 * century)) % 7U;
		uint32_t day = rule.day < dow ? rule.day + 7U - dow : rule.day - dow;
		for (uint16_t i = 1U; i < rule.week; ++i)
		{
			if (day + 7U >= daysFor[_month])
				break;
			day += 7U;
		}
		time += durationIn<seconds>(days{day});
		break;
	}

	rule.change = time - rule.offset + rule.seconds;
	rule.yearFor = year;
}

ormDateTime_t::timezone_t ormDateTime_t::tzComputeFor(const time_t timeSecs, const int16_t year) noexcept
{
	computeChange(tzRules[0], year);
	computeChange(tzRules[1], year);
	if (tzRules[0].change > tzRules[1].change)
		isDaylight = timeSecs < tzRules[1].change || timeSecs >= tzRules[0].change;
	else
		isDaylight = timeSecs >= tzRules[0].change && timeSecs < tzRules[1].change;
	return {tzRules[isDaylight].offset, 0, 0};
}

void ormDateTime_t::tzComputeLeaps(ormDateTime_t::timezone_t &result, const time_t timeSecs) noexcept
{
	size_t index{leapsCount};
	do
	{
		if (!index)
			return;
		--index;
	}
	while (timeSecs < leaps[index].transition);
	result.leapCorrection = leaps[index].change;
	if (timeSecs == leaps[index].transition && ((!index && leaps[index].change > 0) ||
		leaps[index].change > leaps[index - 1].change))
	{
		while (index > 0 && leaps[index].transition == leaps[index - 1].transition + 1 &&
			leaps[index].change == leaps[index - 1].change + 1)
		{
			++result.leapCount;
			--index;
		}
		++result.leapCount;
	}
}

int16_t ormDateTime_t::computeOffsetYear(const systemTime_t time, const seconds offset) noexcept
{
	auto day = days{durationAs<seconds>(time) / seconds{1_day}};
	auto rem = time - day;
	rem += offset;
	correctDay(day, rem);
	return computeYear(day);
}

ormDateTime_t::timezone_t ormDateTime_t::tzCompute(const systemTime_t &time) noexcept
{
	if (!tzInitialised)
		tzInit();

	const time_t timeSecs{systemClock_t::to_time_t(timePoint_t{time})};
	size_t typeIndex{0};
	tzName[0] = nullptr;
	tzName[1] = nullptr;
	if (!transitionsCount || timeSecs < transitions[0U])
	{
		for (; typeIndex < typesCount && ::types[typeIndex].isDst; ++typeIndex)
		{
			if (!tzName[1])
				tzName[1] = tzString(&zoneNames[::types[typeIndex].index]);
		}
		if (typesCount && typeIndex == typesCount)
			tzName[0] = tzString(&zoneNames[::types[typeIndex = 0U].index]);
		for (size_t j{typeIndex}; j < typesCount && !tzName[1U]; ++j)
		{
			if (::types[j].isDst)
				tzName[1] = tzString(&zoneNames[::types[j].index]);
		}
	}
	else if (timeSecs >= transitions[transitionsCount - 1U])
	{
		if (!tzSpec)
			typeIndex = computeRules(transitionsCount - 1U);
		else
		{
			tzParseSpec();
			const auto year = computeOffsetYear(time, 0_sec);
			auto result = tzComputeFor(timeSecs, year);
			tzComputeLeaps(result, timeSecs);
			return result;
		}
	}
	else
		typeIndex = computeRules(searchFor(timeSecs) - 1U);
	timezone_t result{computeOffset(typeIndex), 0, 0};
	tzComputeLeaps(result, timeSecs);
	return result;
}
