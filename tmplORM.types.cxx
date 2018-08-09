#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <array>
#include <chrono>
#include <tmplORM.types.hxx>
#include <fd.hxx>

using tmplORM::types::baseTypes::ormDateTime_t;

constexpr static std::array<char, 4> tzMagic{{'T', 'Z', 'i', 'f'}};
struct tzHead_t
{
	std::array<char, 4> magic;
	char version;
	std::array<char, 15> reserved;
	std::array<char, 4> ttIsGmtCount;
	std::array<char, 4> ttIsStdCount;
	std::array<char, 4> leapCount;
	std::array<char, 4> timeCount;
	std::array<char, 4> typeCount;
	std::array<char, 4> charCount;
};

struct ttInfo_t
{
	int32_t offset;
	uint8_t isDst;
	uint8_t index;
	bool isStd;
	bool isGmt;
};

struct leap_t
{
	time_t transition;
	int32_t change;
};

enum class tzType_t { J0, J1, M };

struct tzRule_t
{
	std::unique_ptr<char []> name;
	tzType_t type;
	uint16_t m, n, d;
	int32_t secs;
	int64_t offset;
	time_t change;
	int32_t computedFor;
};

struct tzString_t
{
	size_t length;
	std::unique_ptr<char []> data;
	std::unique_ptr<tzString_t> next;
};

//std::array<tzRule_t, 2> tzRules{};
static std::array<const char *, 2> tzName{};
static int32_t ruleStdOffset{};
static int32_t ruleDstOffset{};
static bool isDaylight;
static int32_t timeZone;
static bool tzInitialised{false};

static size_t transitionsCount{}, typesCount{}, leapsCount{};
static std::unique_ptr<time_t []> transitions{};
static std::unique_ptr<uint8_t []> typeIndexes{};
static std::unique_ptr<ttInfo_t []> types{};
static std::unique_ptr<char []> zoneNames{};
static std::unique_ptr<leap_t []> leaps{};
static std::unique_ptr<char []> tzSpec{};
static std::unique_ptr<tzString_t> tzStringList{};

// TODO: fixme.
#define TZDIR "/usr/share/zoneinfo"
#define TZDEFAULT "localtime"

template<typename T> constexpr int signBit() noexcept
	{ return std::numeric_limits<typename std::make_signed<T>::type>::digits; }
constexpr size_t sizeMax = std::numeric_limits<size_t>::max();
constexpr uint8_t sizeBits = std::numeric_limits<size_t>::digits - 1;

inline int32_t asInt32(const uint8_t *const value) noexcept
{
	int32_t result{};
	if (sizeof(int32_t) != 4 && value[0] >> signBit<char>())
		result = -1 & ~0xFFFFFFFF;
	result |= (int32_t{value[0]} << 24) | (int32_t{value[1]} << 16) |
		(int32_t{value[2]} << 8) | int32_t{value[3]};
	return result;
}
inline int32_t asInt32(const char *const value) noexcept
	{ return asInt32(reinterpret_cast<const uint8_t *>(value)); }
inline int32_t asInt32(const std::array<char, 4> &value) noexcept
	{ return asInt32(value.data()); }
inline int32_t asInt32(const std::array<uint8_t, 4> &value) noexcept
	{ return asInt32(value.data()); }

inline int64_t asInt64(const uint8_t *const value) noexcept
{
	int64_t result{};
	if (sizeof(int64_t) != 8 && value[0] >> signBit<char>())
		result = -1 & ~0xFFFFFFFFFFFFFFFF;
	result |= (int64_t{value[0]} << 56) | (int64_t{value[1]} << 48) |
		(int64_t{value[2]} << 40) | (int64_t{value[3]} << 32) |
		(int64_t{value[4]} << 24) | (int64_t{value[5]} << 16) |
		(int64_t{value[6]} << 8) | int64_t{value[7]};
	return result;
}
inline int64_t asInt64(const char *const value) noexcept
	{ return asInt64(reinterpret_cast<const uint8_t *>(value)); }

inline int64_t asInt64(const char *const value, const size_t width) noexcept
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
	else if ((a >> (sizeBits - 1)) + (b >> (sizeBits - 1)) > 3)
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
	printf("Something went wrong.. %d: %s\n", errno, strerror(errno));
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
	if (!fd.read(transitions.get(), transitionsCount * width) ||
		!fd.read(typeIndexes, transitionsCount))
		return false;

	const char *const buffer = reinterpret_cast<char *>(transitions.get());
	if (sizeof(time_t) == 8)
	{
		for (size_t i{transitionsCount}; i > 0; )
		{
			size_t index = --i;
			transitions[index] = asInt64(buffer + (i * width), width);
			if (typeIndexes[i] >= typesCount)
				return false;
		}
	}
	else if (width == 4)
	{
		for (size_t i{transitionsCount}; i > 0; )
		{
			size_t index = --i;
			transitions[index] = asInt32(buffer + (i * width));
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
		std::array<uint8_t, 4> offset;
		uint8_t value;

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
	char buffer[8];
	for (size_t i{}; i < leapsCount; ++i)
	{
		if (!fd.read(buffer, width) ||
			!fd.read(leaps[i].change))
			return false;
		else if (sizeof(time_t) == 8)
			leaps[i].transition = asInt64(buffer, width);
		else
			leaps[i].transition = asInt32(buffer);
		swapBytes(leaps[i].change);
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
			tzSpec[tzSpecLen] = 0;
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

	auto value = makeUnique<tzString_t>();
	if (!value)
		return nullptr;
	value->data = makeUnique<char []>(length + 1);
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

bool registerZones(const size_t charCount) noexcept
{
	zoneNames[charCount] = 0;
	for (size_t i{}; i < transitionsCount; ++i)
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

	size_t charCount{}, isStdCount{}, isGmtCount{};
	int8_t version{};
	auto readHeader = [&]() noexcept -> bool
	{
		tzHead_t header;
		if (!fd.read(header) || header.magic != tzMagic)
			return false;
		transitionsCount = asInt32(header.timeCount);
		typesCount = asInt32(header.typeCount);
		charCount = asInt32(header.charCount);
		leapsCount = asInt32(header.leapCount);
		isStdCount = asInt32(header.ttIsStdCount);
		isGmtCount = asInt32(header.ttIsGmtCount);
		if (isStdCount > typesCount || isGmtCount > typesCount)
			return false;
		version = header.version;
		return true;
	};

	if (!readHeader())
		return false;
	else if (sizeof(time_t) == 8 && version)
	{
		const size_t toSkip{safeAdd(safeMul(transitionsCount, 5), safeMul(typesCount, 6),
			charCount, safeMul(leapsCount, 8), isStdCount, isGmtCount)};
		if (toSkip == sizeMax || !fd.seek(toSkip, SEEK_CUR) || !readHeader())
			return false;
		width = 8;
	}
	else if (sizeof(time_t) == 4 && width == 8)
		return false;

	const size_t totalSize{safeAdd(safeMul(transitionsCount, width + 1), safeMul(typesCount, 6),
		charCount, safeMul(leapsCount, 8), isStdCount, isGmtCount)};
	if (totalSize == sizeMax)
		return false;
	size_t tzSpecLen{};
	if (sizeof(time_t) == 8 && width == 8)
	{
		const off_t rem = fileStat.st_size - fd.tell();
		if (rem < 0)
			return false;
		tzSpecLen = safeSub(rem, totalSize, 1);
		if (tzSpecLen == 0 || tzSpecLen == sizeMax)
			return false;
	}

	transitions = makeUnique<time_t []>(transitionsCount);
	typeIndexes = makeUnique<uint8_t []>(transitionsCount);
	types = makeUnique<ttInfo_t []>(typesCount);
	zoneNames = makeUnique<char []>(charCount + 1);
	leaps = makeUnique<leap_t []>(leapsCount);
	tzSpec = tzSpecLen ? makeUnique<char []>(tzSpecLen) : nullptr;
	if (!transitions || !typeIndexes || !types ||
		!zoneNames || !leaps || (tzSpecLen && !tzSpec) ||
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
	tzName[1] = tzName[1];
	tzInitialised = true;
}

void ormDateTime_t::tzCompute(const systemTime_t &time)
{
	if (!tzInitialised)
		tzInit();
}
