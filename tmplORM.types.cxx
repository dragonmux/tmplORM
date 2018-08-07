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
