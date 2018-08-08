#include <crunch++.h>
#include <tmplORM.types.cxx>

class testDateTime_t final : public testsuit
{
private:
	void testReadExtended()
	{
		tzReadFile("data/GTM_BST.timezone");
		assertEqual(tzName[0], "GMT");
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
