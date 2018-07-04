#include <chrono>
#include <crunch++.h>
#include <tmplORM.types.hxx>
#include "testTypes.hxx"

using namespace tmplORM::types::baseTypes;
using systemClock_t = std::chrono::system_clock;

namespace dateTime
{
	void testCtor(testsuit &suite)
	{
		const ormDateTime_t a{};
		suite.assertEqual(a.year(), 0);
		suite.assertEqual(a.month(), 0);
		suite.assertEqual(a.day(), 0);
		suite.assertEqual(a.hour(), 0);
		suite.assertEqual(a.minute(), 0);
		suite.assertEqual(a.second(), 0);
		suite.assertEqual(a.nanoSecond(), 0);

		const ormDateTime_t b{2018, 07, 04, 12, 34, 45, 678901234};
		suite.assertEqual(b.year(), 2018);
		suite.assertEqual(b.month(), 07);
		suite.assertEqual(b.day(), 04);
		suite.assertEqual(b.hour(), 12);
		suite.assertEqual(b.minute(), 34);
		suite.assertEqual(b.second(), 45);
		suite.assertEqual(b.nanoSecond(), 678901234);
	}

	void testFromSystemTime(testsuit &suite)
	{
		ormDateTime_t now = systemClock_t::now();
	}
}
