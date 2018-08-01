#include <chrono>
#include <crunch++.h>
#include <tmplORM.hxx>
#include "testTypes.hxx"

using systemClock_t = std::chrono::system_clock;

namespace dateTime
{
	using namespace tmplORM::types::baseTypes;
	using namespace tmplORM::types::dateTimeTypes;

	void testCtor(testsuit &suite)
	{
		const ormDateTime_t a;
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

	void testFromString(testsuit &suite)
	{
		const ormDateTime_t a{"2018-07-04 12:34:45.678901234"};
		suite.assertEqual(a.year(), 2018);
		suite.assertEqual(a.month(), 07);
		suite.assertEqual(a.day(), 04);
		suite.assertEqual(a.hour(), 12);
		suite.assertEqual(a.minute(), 34);
		suite.assertEqual(a.second(), 45);
		suite.assertEqual(a.nanoSecond(), 678901234);
		const ormDateTime_t b{"2018-07-04 12:34:45.6789"};
		suite.assertEqual(b.year(), 2018);
		suite.assertEqual(b.month(), 07);
		suite.assertEqual(b.day(), 04);
		suite.assertEqual(b.hour(), 12);
		suite.assertEqual(b.minute(), 34);
		suite.assertEqual(b.second(), 45);
		suite.assertEqual(b.nanoSecond(), 678900000);
		const ormDateTime_t c{"2018-07-04 12:34:45.6789012345678"};
		suite.assertEqual(c.year(), 2018);
		suite.assertEqual(c.month(), 07);
		suite.assertEqual(c.day(), 04);
		suite.assertEqual(c.hour(), 12);
		suite.assertEqual(c.minute(), 34);
		suite.assertEqual(c.second(), 45);
		suite.assertEqual(c.nanoSecond(), 678901234);
	}

	void testFromSystemTime(testsuit &suite)
	{
		using seconds = std::chrono::seconds;
		const auto now = systemClock_t::now();
		const ormDateTime_t a{now};

		const ::time_t time = systemClock_t::to_time_t(now);
		const tm local = *gmtime(&time);

		suite.assertEqual(a.year(), local.tm_year + 1900);
		suite.assertEqual(a.month(), local.tm_mon + 1);
		suite.assertEqual(a.day(), local.tm_mday);
		suite.assertEqual(a.hour(), local.tm_hour);
		suite.assertEqual(a.minute(), local.tm_min);
		suite.assertEqual(a.second(), local.tm_sec);
		suite.assertEqual(a.nanoSecond(), (now.time_since_epoch() - seconds{time}).count());
	}

	void testWrapper(testsuit &suite)
	{
		const _dateTime_t a;
		suite.assertEqual(a.year(), 0);
		suite.assertEqual(a.month(), 0);
		suite.assertEqual(a.day(), 0);
		suite.assertEqual(a.time().count(), 0);
		const _dateTime_t b{2018, 07, 04};
		suite.assertEqual(b.year(), 2018);
		suite.assertEqual(b.month(), 07);
		suite.assertEqual(b.day(), 04);
		suite.assertEqual(b.time().count(), 0);
		const _dateTime_t c{2018, 07, 04, nanoseconds{1234567890}};
		suite.assertEqual(c.year(), 2018);
		suite.assertEqual(c.month(), 07);
		suite.assertEqual(c.day(), 04);
		suite.assertEqual(c.time().count(), 1234567890);
	}
}

namespace types
{
	using irqus::typestring;
	using date_t = tmplORM::types::date_t<typestring<>>;
	using time_t = tmplORM::types::time_t<typestring<>>;
	using dateTime_t = tmplORM::types::dateTime_t<typestring<>>;
	using uuid_t = tmplORM::types::uuid_t<typestring<>>;
	using namespace tmplORM::types::baseTypes;

	using std::chrono::milliseconds;
	using tmplORM::types::chrono::durationIn;

	const auto now = ormDateTime_t{systemClock_t::now()};

	const auto timeUUID = []() -> ormUUID_t
	{
		// Set up our UUID value.
		const auto now = systemClock_t::now().time_since_epoch();
		const uint64_t time = durationIn<milliseconds>(now);
		const auto nanoSeconds = (now - milliseconds{time}).count();
		return {uint32_t(time), uint16_t(time >> 32),
			uint16_t(0x1000 | ((time >> 48) & 0x0FFF)),
			uint16_t((nanoSeconds >> 14) | 0x8000), swapBytes(uint64_t{0x123456789ABCU}) >> 16};
	}();

	void testDate(testsuit &suite)
	{
		date_t date;
		const ormDate_t a = date.value();
		suite.assertEqual(a.year(), 0);
		suite.assertEqual(a.month(), 0);
		suite.assertEqual(a.day(), 0);

		date.value(now);
		suite.assertTrue(date.value() == now);
		suite.assertTrue(date.date() == now);

		date = ormDate_t{};
		const ormDate_t b = date;
		suite.assertEqual(b.year(), 0);
		suite.assertEqual(b.month(), 0);
		suite.assertEqual(b.day(), 0);

		date = "2018-07-04";
		const ormDate_t c = date;
		suite.assertEqual(c.year(), 2018);
		suite.assertEqual(c.month(), 07);
		suite.assertEqual(c.day(), 04);

		date = {};
		const ormDate_t d = date;
		suite.assertEqual(d.year(), 0);
		suite.assertEqual(d.month(), 0);
		suite.assertEqual(d.day(), 0);

		date= now;
		date = nullptr;
		const ormDate_t e = date;
		suite.assertEqual(e.year(), 0);
		suite.assertEqual(e.month(), 0);
		suite.assertEqual(e.day(), 0);
	}

	void testTime(testsuit &suite)
	{
		time_t time;
		const ormTime_t a = time.value();
		suite.assertEqual(a.hour(), 0);
		suite.assertEqual(a.minute(), 0);
		suite.assertEqual(a.second(), 0);
		suite.assertEqual(a.nanoSecond(), 0);

		time.value(now);
		suite.assertTrue(time.value() == now);
		suite.assertTrue(time.time() == now);

		time = ormTime_t{};
		const ormTime_t b = time;
		suite.assertEqual(b.hour(), 0);
		suite.assertEqual(b.minute(), 0);
		suite.assertEqual(b.second(), 0);
		suite.assertEqual(b.nanoSecond(), 0);

		time = "12:34:56:789012345";
		const ormTime_t c = time;
		suite.assertEqual(c.hour(), 12);
		suite.assertEqual(c.minute(), 34);
		suite.assertEqual(c.second(), 56);
		suite.assertEqual(c.nanoSecond(), 789012345);

		time = {};
		const ormTime_t d = time;
		suite.assertEqual(d.hour(), 0);
		suite.assertEqual(d.minute(), 0);
		suite.assertEqual(d.second(), 0);
		suite.assertEqual(d.nanoSecond(), 0);

		time = now;
		time = nullptr;
		const ormTime_t e = time;
		suite.assertEqual(e.hour(), 0);
		suite.assertEqual(e.minute(), 0);
		suite.assertEqual(e.second(), 0);
		suite.assertEqual(e.nanoSecond(), 0);
	}

	void testDateTime(testsuit &suite)
	{
		dateTime_t dateTime;
		const ormDateTime_t a = dateTime.value();
		suite.assertEqual(a.year(), 0);
		suite.assertEqual(a.month(), 0);
		suite.assertEqual(a.day(), 0);
		suite.assertEqual(a.hour(), 0);
		suite.assertEqual(a.minute(), 0);
		suite.assertEqual(a.second(), 0);
		suite.assertEqual(a.nanoSecond(), 0);

		dateTime.value(now);
		suite.assertTrue(dateTime.value() == now);
		suite.assertTrue(dateTime.dateTime() == now);

		dateTime = ormDateTime_t{};
		const ormDateTime_t b = dateTime;
		suite.assertEqual(b.year(), 0);
		suite.assertEqual(b.month(), 0);
		suite.assertEqual(b.day(), 0);
		suite.assertEqual(b.hour(), 0);
		suite.assertEqual(b.minute(), 0);
		suite.assertEqual(b.second(), 0);
		suite.assertEqual(b.nanoSecond(), 0);

		dateTime = "2018-07-04 12:34:56:789012345";
		const ormDateTime_t c = dateTime;
		suite.assertEqual(c.year(), 2018);
		suite.assertEqual(c.month(), 07);
		suite.assertEqual(c.day(), 04);
		suite.assertEqual(c.hour(), 12);
		suite.assertEqual(c.minute(), 34);
		suite.assertEqual(c.second(), 56);
		suite.assertEqual(c.nanoSecond(), 789012345);

		dateTime = {};
		const ormDateTime_t d = dateTime;
		suite.assertEqual(d.year(), 0);
		suite.assertEqual(d.month(), 0);
		suite.assertEqual(d.day(), 0);
		suite.assertEqual(d.hour(), 0);
		suite.assertEqual(d.minute(), 0);
		suite.assertEqual(d.second(), 0);
		suite.assertEqual(d.nanoSecond(), 0);

		dateTime = now;
		dateTime = nullptr;
		const ormDateTime_t e = dateTime;
		suite.assertEqual(e.year(), 0);
		suite.assertEqual(e.month(), 0);
		suite.assertEqual(e.day(), 0);
		suite.assertEqual(e.hour(), 0);
		suite.assertEqual(e.minute(), 0);
		suite.assertEqual(e.second(), 0);
		suite.assertEqual(e.nanoSecond(), 0);
	}

	void testUUID(testsuit &suite)
	{
		suite.assertEqual(sizeof(uint64_t), 8);

		uuid_t uuid;
		const ormUUID_t a = uuid.value();
		suite.assertEqual(a.data1(), 0);
		suite.assertEqual(a.data2(), 0);
		suite.assertEqual(a.data3(), 0);
		suite.assertEqual(a.data4(), "\x00\x00\x00\x00\x00\x00\x00\x00", 8);

		uuid.value(timeUUID);
		suite.assertTrue(uuid.value() == timeUUID);
		suite.assertTrue(uuid.uuid() == timeUUID);

		uuid = ormUUID_t{};
		const ormUUID_t b = uuid;
		suite.assertEqual(b.data1(), 0);
		suite.assertEqual(b.data2(), 0);
		suite.assertEqual(b.data3(), 0);
		suite.assertEqual(b.data4(), "\x00\x00\x00\x00\x00\x00\x00\x00", 8);

		uuid = timeUUID;
		uuid = {};
		suite.assertEqual(b.data1(), 0);
		suite.assertEqual(b.data2(), 0);
		suite.assertEqual(b.data3(), 0);
		suite.assertEqual(b.data4(), "\x00\x00\x00\x00\x00\x00\x00\x00", 8);
	}
}
