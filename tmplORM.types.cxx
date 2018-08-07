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
	uint8_t idx;
	uint8_t isStd;
	uint8_t isGmt;
};

struct leap_t
{
	time_t transition;
	long change;
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
std::unique_ptr<char []> typeIndexes{};
std::unique_ptr<ttInfo_t []> types{};
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
inline int32_t asInt32(const std::array<char, 4> &value) noexcept
	{ return asInt32(reinterpret_cast<const uint8_t *>(value.data())); }

inline int64_t asInt64(const char *const value) noexcept
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
inline int32_t asInt64(const std::array<char, 8> &value) noexcept
	{ return asInt64(value.data()); }

void tzReadFile(const char *const file)
{
	fd_t fd{};
	uint8_t width = 4;

	static_assert(sizeof(time_t) == 4 || sizeof(time_t) == 8, "time_t not a valid size");
	if (file[0] != '/')
	{
		const char *dir = getenv("TZDIR");
		if (!dir || !dir[0])
			dir = TZDIR;
		auto fileName = formatString("%s/%s", dir, file);
		if (!fileName)
			return;
		fd = {fileName.get(), O_RDONLY | O_CLOEXEC};
	}
	else
		fd = {file, O_RDONLY | O_CLOEXEC};

	const auto fileStat = fd.stat();
	if (fileStat.st_size == 0)
		return;

	tzHead_t header;
	if (!fd.read(header) || header.magic != tzMagic)
		return;

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
	if (tzInitialised)
		tzInit();
}
