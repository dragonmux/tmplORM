#ifndef DATETIME_INTERNAL_HXX
#define DATETIME_INTERNAL_HXX

#include <time.h>
#include <array>
#include <memory>

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

extern std::array<const char *, 2> tzName;
extern bool tzInitialised;

extern std::unique_ptr<time_t []> transitions;
extern std::unique_ptr<uint8_t []> typeIndexes;
extern std::unique_ptr<ttInfo_t []> types;
extern std::unique_ptr<char []> zoneNames;
extern std::unique_ptr<leap_t []> leaps;
extern std::unique_ptr<char []> tzSpec;

extern bool tzReadFile(const char *const file) noexcept;

#endif /*DATETIME_INTERNAL_HXX*/
