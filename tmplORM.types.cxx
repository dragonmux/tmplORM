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

std::array<tzRule_t, 2> tzRules{};
size_t transitionsCount{}, typesCount{}, leapsCount{};
std::unique_ptr<time_t []> transitions{};
std::unique_ptr<uint8_t []> typeIndexes{};
std::unique_ptr<ttInfo_t []> types{};
std::unique_ptr<char []> zoneNames{};
std::unique_ptr<leap_t []> leaps{};

// TODO: fixme.
#define TZDIR "/usr/share/zoneinfo"
#define TZDEFAULT "localtime"
static bool tzInitialised = false;

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

inline void badRead() noexcept
{
	printf("Something went wrong.. %d: %s\n", errno, strerror(errno));
	transitions = nullptr;
	typeIndexes = nullptr;
	types = nullptr;
	zoneNames = nullptr;
	leaps = nullptr;
}

fd_t tzOpenFile(const char *const file)
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

void tzReadFile(const char *const file)
{
	uint8_t width = 4;

	static_assert(sizeof(time_t) == 4 || sizeof(time_t) == 8, "time_t not a valid size");
	fd_t fd{tzOpenFile(file)};
	if (!fd.valid())
		return;
	const auto fileStat = fd.stat();
	if (fileStat.st_size == 0)
		return;

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
		return;
	else if (sizeof(time_t) == 8 && version)
	{
		width = 8;
		const size_t toSkip = transitionsCount * 5 + typesCount * 6 +
			charCount + leapsCount * 8 + isStdCount + isGmtCount;
		if (!fd.seek(toSkip, SEEK_CUR))
			return;
		else if (!readHeader())
			return;
	}
	size_t dataOffset = fd.tell();
	size_t totalSize{safeMul(transitionsCount, sizeof(time_t) + 1)};
	//totalSize = safeAnd(safeAdd(totalSize, alignof(ttInfo_t) - 1), ~(alignof(ttInfo_t) - 1));
	if (totalSize >= sizeMax - dataOffset)
		return;
	const size_t typesOffset = totalSize + dataOffset;
	totalSize = safeAdd(safeMul(typesCount, sizeof(ttInfo_t)), totalSize);
	totalSize = safeAdd(totalSize, charCount);
	//totalSize = safeAnd(safeAdd(totalSize, alignof(leap_t) - 1), ~(alignof(leap_t) - 1));
	if (totalSize >= sizeMax - dataOffset)
		return;
	const size_t leapsOffset = totalSize + dataOffset;
	totalSize = safeAdd(safeMul(leapsCount, sizeof(leap_t)), totalSize);

	size_t tzSpecLen{};
	if (sizeof(time_t) == 8 && width == 8)
	{
		const off_t rem = fileStat.st_size - fd.tell();
		if (rem < 0)
			return;
		tzSpecLen = safeAdd(safeAdd(safeMul(transitionsCount, 9),
			safeMul(typesCount, 6)), charCount);
		tzSpecLen = safeSub(rem, tzSpecLen);
		tzSpecLen = safeSub(tzSpecLen, safeMul(leapsCount, 12));
		tzSpecLen = safeSub(tzSpecLen, isStdCount);
		tzSpecLen = safeSub(tzSpecLen, isGmtCount + 1);
		if (tzSpecLen == 0 || tzSpecLen == sizeMax)
			return;
	}
	if (safeAdd(totalSize, tzSpecLen) == sizeMax)
		return;
	transitions = makeUnique<time_t []>(transitionsCount);
	typeIndexes = makeUnique<char []>(transitionsCount);
	types = makeUnique<ttInfo_t []>(typesCount);
	leaps = makeUnique<leap_t []>(leapsCount);
	if (!transitions || !typeIndexes || !types || !leaps)
		return badRead();
	else if (!fd.read(transitions, transitionsCount) ||
		!fd.read(typeIndexes, transitionsCount))
		return badRead();
	else if (!fd.seek(typesOffset, SEEK_SET) || !fd.read(types, typesCount))
		return badRead();
	else if (!fd.seek(leapsOffset, SEEK_SET) || !fd.read(leaps, leapsCount))
		return badRead();

	tzInitialised = true;
}

void tzInit()
{
	const char *tz = getenv("TZ");
	if (!tz)
		tz = TZDEFAULT;
	else if (!tz[0])
		tz = "Universal";
	else if (tz[0] == ':')
		++tz;

	tzReadFile(tz);
}

void ormDateTime_t::tzCompute(const systemTime_t &time)
{
	if (!tzInitialised)
		tzInit();
}
