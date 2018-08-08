#include <stdlib.h>
#include <crunch++.h>
#include <tmplORM.types.cxx>

class testDateTime_t final : public testsuit
{
private:
	void testReadExtended()
	{
		assertEqual(setenv("TZDIR", getenv("PWD"), true), 0);

		tzReadFile("data/GMT_BST.timezone");
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

public:
	void registerTests() final override
	{
		CXX_TEST(testReadExtended)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testDateTime_t>();
}
