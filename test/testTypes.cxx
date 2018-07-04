#include <crunch++.h>
#include "testTypes.hxx"

class testDateTime_t final : public testsuit
{
private:
	void testCtor() { dateTime::testCtor(*this); }

public:
	void registerTests() final override
	{
		CXX_TEST(testCtor)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testDateTime_t>();
}
