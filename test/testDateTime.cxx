#include <stdlib.h>
#include <time.h>
#include <crunch++.h>
#include <tmplORM.types.cxx>

using systemClock_t = std::chrono::system_clock;

std::unique_ptr<char []> currentWorkDir() noexcept
{
	const char *cwd = getcwd(nullptr, 0);
	if (!cwd)
		return nullptr;
	return stringDup(cwd);
}

class testDateTime_t final : public testsuit
{
private:
	void testReadExtended()
	{
		auto cwd = currentWorkDir();
		assertNotNull(cwd);
		assertEqual(setenv("TZDIR", cwd.get(), true), 0);

		assertTrue(tzReadFile("data/GMT_BST.timezone"));
		assertTrue(tzInitialised);
		tzInitialised = false;
		assertNotNull(transitions);
		assertNotNull(typeIndexes);
		assertNotNull(types);
		assertNotNull(zoneNames);
		assertNotNull(leaps);
		assertNotNull(tzSpec);

		assertNotNull(tzName[0]);
		assertEqual(tzName[0], "GMT");
		assertNotNull(tzName[1]);
		assertEqual(tzName[1], "BST");
	}

	void testConversion()
	{
		const auto now{systemClock_t::now()};
		const auto nowSecs{systemClock_t::to_time_t(now)};
		const auto localTime{*localtime(&nowSecs)};
		const ormDateTime_t ormTime{now};

		assertEqual(ormTime.year(), localTime.tm_year + 1900);
		assertEqual(ormTime.month(), localTime.tm_mon + 1);
		assertEqual(ormTime.day(), localTime.tm_mday);
		assertEqual(ormTime.hour(), localTime.tm_hour);
		assertEqual(ormTime.minute(), localTime.tm_min);
		assertEqual(ormTime.second(), localTime.tm_sec);
	}

public:
	void registerTests() final override
	{
		CXX_TEST(testReadExtended)
		CXX_TEST(testConversion)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testDateTime_t>();
}
