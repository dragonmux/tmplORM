#ifndef tmplORM_TYPES__HXX
#define tmplORM_TYPES__HXX

#include "conversions.hxx"

namespace tmplORM
{
	namespace types
	{
		inline namespace baseTypes
		{
			struct ormDate_t
			{
			protected:
				uint16_t _year;
				uint16_t _month;
				uint16_t _day;

			public:
				constexpr ormDate_t() noexcept : _year(0), _month(0), _day(0) { }
				constexpr ormDate_t(const uint16_t year, const uint16_t month, const uint16_t day) noexcept :
					_year(year), _month(month), _day(day) { }

				constexpr uint16_t year() const noexcept { return _year; }
				constexpr uint16_t month() const noexcept { return _month; }
				constexpr uint16_t day() const noexcept { return _day; }

				ormDate_t(const char *date) noexcept : ormDate_t()
				{
					_year = toInt_t<uint16_t>(date, 4);
					date += 5;
					_month = toInt_t<uint16_t>(date, 2);
					date += 3;
					_day = toInt_t<uint16_t>(date, 2);
				}
			};

			struct ormDateTime_t final : public ormDate_t
			{
			private:
				uint16_t _hour;
				uint16_t _minute;
				uint16_t _second;
				uint32_t _nanoSecond;

				size_t power10(const size_t power) const noexcept
				{
					size_t ret = 1;
					for (size_t i = 0; i < power; ++i)
						ret *= 10;
					return ret;
				}

			public:
				constexpr ormDateTime_t() noexcept : ormDate_t(), _hour(0), _minute(0), _second(0), _nanoSecond(0) { }
				constexpr ormDateTime_t(const uint16_t year, const uint16_t month, const uint16_t day,
					const uint16_t hour, const uint16_t minute, const uint16_t second, const uint32_t nanoSecond) :
					ormDate_t(year, month, day), _hour(hour), _minute(minute), _second(second), _nanoSecond(nanoSecond) { }

				constexpr uint16_t hour() const noexcept { return _hour; }
				constexpr uint16_t minute() const noexcept { return _minute; }
				constexpr uint16_t second() const noexcept { return _second; }
				constexpr uint32_t nanoSecond() const noexcept { return _nanoSecond; }

				ormDateTime_t(const char *dateTime) noexcept : ormDate_t(dateTime), _hour(0), _minute(0), _second(0), _nanoSecond(0)
				{
					dateTime += 11;
					_hour = toInt_t<uint16_t>(dateTime, 2);
					dateTime += 3;
					_minute = toInt_t<uint16_t>(dateTime, 2);
					dateTime += 3;
					_second = toInt_t<uint16_t>(dateTime, 2);
					dateTime += 3;
					toInt_t<uint32_t> nanoSeconds(dateTime);
					if (nanoSeconds.length() <= 9)
						_nanoSecond = nanoSeconds * power10(9 - nanoSeconds.length());
					else
						_nanoSecond = nanoSeconds / power10(nanoSeconds.length() - 9);
				}
			};
		}
	}
}

#endif /*tmplORM_TYPES__HXX*/
