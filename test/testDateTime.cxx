#include <stdlib.h>
#include <crunch++.h>
#include <tmplORM.types.cxx>

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
