#include <crunch++.h>
#include <mysql.hxx>

class testMySQL final : public testsuit
{
public:
	void registerTests() final override
	{
		//CXX_TEST(testValidity)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testMySQL>();
}
