#ifndef tmplORM_TYPES__HXX
#define tmplORM_TYPES__HXX

#include <conversions.hxx>

/*!
 * @file
 * @author Rachel Mant
 * @date 2017
 * @brief Defines core base types for the ORM
 */

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

				friend bool operator ==(const ormDate_t &a, const ormDate_t &b) noexcept;

			public:
				constexpr ormDate_t() noexcept : _year{0}, _month{0}, _day{0} { }
				constexpr ormDate_t(const uint16_t year, const uint16_t month, const uint16_t day) noexcept :
					_year{year}, _month{month}, _day{day} { }

				constexpr uint16_t year() const noexcept { return _year; }
				void year(const uint16_t year) noexcept { _year = year; }
				constexpr uint16_t month() const noexcept { return _month; }
				void month(const uint16_t month) noexcept { _month = month; }
				constexpr uint16_t day() const noexcept { return _day; }
				void day(const uint16_t day) noexcept { _day = day; }

				ormDate_t(const char *date) noexcept : ormDate_t{}
				{
					_year = toInt_t<uint16_t>(date, 4);
					date += 5;
					_month = toInt_t<uint16_t>(date, 2);
					date += 3;
					_day = toInt_t<uint16_t>(date, 2);
				}
			};

			inline bool operator ==(const ormDate_t &a, const ormDate_t &b) noexcept
				{ return a._year == b._year && a._month == b._month && a._day == b._day; }
			inline bool operator !=(const ormDate_t &a, const ormDate_t &b) noexcept { return !(a == b); }

			struct ormTime_t
			{
			protected:
				uint16_t _hour;
				uint16_t _minute;
				uint16_t _second;
				uint32_t _nanoSecond;

				/*!
				* @brief Raise 10 to the power of power.
				* @note This is intentionally limited to positive natural numbers.
				*/
				size_t power10(const size_t power) noexcept
					{ return power ? power10(power - 1) * 10 : 1; }

				friend bool operator ==(const ormTime_t &a, const ormTime_t &b) noexcept;

			public:
				constexpr ormTime_t() noexcept : _hour(0), _minute(0), _second(0), _nanoSecond(0) { }
				constexpr ormTime_t(const uint16_t hour, const uint16_t minute, const uint16_t second,
					const uint32_t nanoSecond) noexcept : _hour{hour}, _minute{minute},
					_second{second}, _nanoSecond{nanoSecond} { }

				constexpr uint16_t hour() const noexcept { return _hour; }
				void hour(const uint16_t hour) noexcept { _hour = hour; }
				constexpr uint16_t minute() const noexcept { return _minute; }
				void minute(const uint16_t minute) noexcept { _minute = minute; }
				constexpr uint16_t second() const noexcept { return _second; }
				void second(const uint16_t second) noexcept { _second = second; }
				constexpr uint32_t nanoSecond() const noexcept { return _nanoSecond; }
				void nanoSecond(const uint32_t nanoSecond) noexcept { _nanoSecond = nanoSecond; }

				ormTime_t(const char *time) noexcept : ormTime_t{}
				{
					_hour = toInt_t<uint16_t>(time, 2);
					time += 3;
					_minute = toInt_t<uint16_t>(time, 2);
					time += 3;
					_second = toInt_t<uint16_t>(time, 2);
					time += 2;
					if (time[0] == 0)
						return;
					++time;
					toInt_t<uint64_t> nanoSeconds(time);
					if (nanoSeconds.length() <= 9)
						_nanoSecond = nanoSeconds * power10(9 - nanoSeconds.length());
					else
						_nanoSecond = nanoSeconds / power10(nanoSeconds.length() - 9);
				}
			};

			inline bool operator ==(const ormTime_t &a, const ormTime_t &b) noexcept
				{ return a._hour == b._hour && a._minute == b._minute &&
						a._second == b._second && a._nanoSecond == b._nanoSecond; }
			inline bool operator !=(const ormTime_t &a, const ormTime_t &b) noexcept { return !(a == b); }

			namespace chrono
			{
				using systemClock_t = std::chrono::system_clock;
				using timePoint_t = systemClock_t::time_point;
				using systemTime_t = timePoint_t::duration;
				template<typename rep_t, typename period_t>
					using duration_t = std::chrono::duration<rep_t, period_t>;
				template<typename target_t, typename source_t>
				constexpr target_t durationAs(const source_t &d)
					{ return std::chrono::duration_cast<target_t>(d); }
				template<typename target_t, typename source_t>
				constexpr typename target_t::rep durationIn(const source_t &d)
					{ return durationAs<target_t>(d).count(); }
				template<std::intmax_t num, std::intmax_t denom = 1>
					using ratio_t = std::ratio<num, denom>;

				using rep_t = systemClock_t::rep;
				using years = duration_t<rep_t, ratio_t<31556952>>;
				using months = duration_t<rep_t, ratio_t<2629746>>;
				using days = duration_t<rep_t, ratio_t<86400>>;
				using hours = std::chrono::hours;
				using minutes = std::chrono::minutes;
				using seconds = std::chrono::seconds;
				using nanoseconds = std::chrono::nanoseconds;

				constexpr years operator ""_y(const unsigned long long value) noexcept { return years{value}; }
				constexpr days operator ""_day(const unsigned long long value) noexcept { return days{value}; }

				//extern "C" void __tz_compute(time_t timer, struct tm *tm, int use_localtime);
				constexpr static std::array<std::array<uint16_t, 12>, 2> monthDays
				{{
					{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
					{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
				}};

				struct ormDateTime_t final : public ormDate_t, ormTime_t
				{
				private:
					struct timezone_t
					{
						int32_t offset;
						int32_t leapCorrection;
						size_t leapCount;
					};

					void display() const noexcept { printf("%04u-%02u-%02u %02u:%02u:%02u.%u\n", _year, _month, _day, _hour, _minute, _second, _nanoSecond); }

					constexpr bool isLeap(const rep_t year) const noexcept
						{ return (year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0); }
					constexpr bool isLeap(const years &year) const noexcept { return isLeap(year.count()); }
					constexpr rep_t div(const years a, const rep_t b) const noexcept
						{ return (a.count() / b) + ((a.count() % b) < 0); }
					constexpr days leapsFor(const years year) const noexcept
						{ return days{div(year, 4) - div(year, 100) + div(year, 400)}; }

					template<typename value_t> void correctDay(days &day, value_t &rem) noexcept
					{
						while (rem.count() < 0)
						{
							rem += 1_day;
							--day;
						}
						while (rem >= 1_day)
						{
							rem -= 1_day;
							++day;
						}
					}

					uint16_t computeYear(days &day) noexcept
					{
						years year = 1970_y;
						while (day.count() < 0 || day.count() > (isLeap(year) ? 366 : 365))
						{
							const years guess = year + years{day.count() / 365} - years{(day.count() % 365) < 0};
							day -= days{(guess - year).count() * 365} + leapsFor(guess - 1_y) - leapsFor(year - 1_y);
							year = guess;
						}
						return year.count();
					}

					uint16_t computeMonth(const uint16_t year, days &day) noexcept
					{
						const auto &daysFor = monthDays[isLeap(year)];
						uint32_t i{0};
						++day;
						while (day.count() > daysFor[i])
							day -= days{daysFor[i++]};
						return i + 1;
					}

					timezone_t tzCompute(const systemTime_t &time);
					friend bool operator ==(const ormDateTime_t &a, const ormDateTime_t &b) noexcept;

				public:
					constexpr ormDateTime_t() noexcept : ormDate_t{}, ormTime_t{} { }
					constexpr ormDateTime_t(const uint16_t year, const uint16_t month, const uint16_t day,
						const uint16_t hour, const uint16_t minute, const uint16_t second, const uint32_t nanoSecond) noexcept :
						ormDate_t{year, month, day}, ormTime_t{hour, minute, second, nanoSecond} { }
					ormDateTime_t(const char *dateTime) noexcept : ormDate_t{dateTime}, ormTime_t{dateTime + 11} { }

					ormDateTime_t(const systemTime_t time) noexcept : ormDateTime_t{}
					{
						timezone_t timeZone = tzCompute(time);
						auto day = days{durationAs<seconds>(time) / seconds{1_day}};
						auto rem = time - day;
						rem += seconds{timeZone.offset};
						correctDay(day, rem);
						_year = computeYear(day);
						_month = computeMonth(_year, day);
						_day = day.count();

						_hour = durationIn<hours>(rem);
						rem -= hours{_hour};
						_minute = durationIn<minutes>(rem);
						rem -= minutes{_minute};
						_second = durationIn<seconds>(rem);
						rem -= seconds{_second};
						_nanoSecond = durationIn<nanoseconds>(rem);

						//display();
					}
					ormDateTime_t(const timePoint_t &point) noexcept :
						ormDateTime_t{point.time_since_epoch()} { }
				};

				inline bool operator ==(const ormDateTime_t &a, const ormDateTime_t &b) noexcept
					{ return ormDate_t(a) == ormDate_t(b) && ormTime_t(a) == ormTime_t(b); }
			}

			using chrono::ormDateTime_t;
			using chrono::operator ==;

			inline bool operator !=(const ormDateTime_t &a, const ormDateTime_t &b) noexcept { return !(a == b); }
			//inline bool operator ==(const ormDate_t &a, const ormDateTime_t &b) noexcept { return a == ormDate_t(b); }
			inline bool operator ==(const ormDateTime_t &a, const ormDate_t &b) noexcept { return ormDate_t{a} == b; }
			inline bool operator ==(const ormDateTime_t &a, const ormTime_t &b) noexcept { return ormTime_t{a} == b; }

			struct guid_t final
			{
				uint32_t data1;
				uint16_t data2;
				uint16_t data3;
				uint64_t data4;
			};

			inline bool operator ==(const guid_t &a, const guid_t &b) noexcept
			{
				return a.data1 == b.data1 && a.data2 == b.data2 &&
					a.data3 == b.data3 && a.data4 == b.data4;
			}

			struct ormUUID_t final
			{
			private:
				/*! @brief union for storing UUIDs - all data must be kept in Big Endian form. */
				guid_t _uuid;
				friend bool operator ==(const ormUUID_t &a, const ormUUID_t &b) noexcept;

			public:
				constexpr ormUUID_t() noexcept : _uuid() { }
				ormUUID_t(const guid_t &guid, const bool needsSwap = false) noexcept : _uuid{guid}
				{
					if (needsSwap)
					{
						swapBytes(_uuid.data1);
						swapBytes(_uuid.data2);
						swapBytes(_uuid.data3);
					}
				}

				ormUUID_t(const uint32_t a, const uint16_t b, const uint16_t c, const uint16_t d,
					const uint64_t e) noexcept : _uuid{a, b, c, uint64_t(e << 16)}
				{
					swapBytes(_uuid.data1);
					swapBytes(_uuid.data2);
					swapBytes(_uuid.data3);
					_uuid.data4 |= swapBytes(d);
				}

				const uint8_t *asBuffer() const noexcept { return reinterpret_cast<const uint8_t *>(&_uuid); }
				uint8_t *asBuffer() noexcept { return reinterpret_cast<uint8_t *>(&_uuid); }
				constexpr uint32_t data1() const noexcept { return _uuid.data1; }
				constexpr uint16_t data2() const noexcept { return _uuid.data2; }
				constexpr uint16_t data3() const noexcept { return _uuid.data3; }
				const uint8_t *data4() const noexcept { return reinterpret_cast<const uint8_t *>(&_uuid.data4); }

				ormUUID_t(const char *uuid) noexcept : ormUUID_t()
				{
					_uuid.data1 = toInt_t<uint32_t>(uuid, 8).fromHex();
					uuid += 9;
					_uuid.data2 = toInt_t<uint16_t>(uuid, 4).fromHex();
					uuid += 5;
					_uuid.data3 = toInt_t<uint16_t>(uuid, 4).fromHex();
					uuid += 5;
					_uuid.data4 = uint64_t(toInt_t<uint16_t>(uuid, 4).fromHex()) << 48;
					uuid += 5;
					_uuid.data4 |= uint64_t(toInt_t<uint16_t>(uuid, 4).fromHex()) << 32;
					_uuid.data4 |= toInt_t<uint32_t>(uuid + 4, 8).fromHex();
				}

				std::unique_ptr<char []> asString() const noexcept
				{
					auto str = makeUnique<char []>(36);
					if (!str)
						return nullptr;
					for (uint8_t i{0}, j{0}; i < 16; ++i)
					{
						// This works because internally we keep things big endian
						const uint8_t value = asBuffer()[i];
						if (i == 4 || i == 6 || i == 8 || i == 10)
							str[j++] = '-';
						str[j++] = asHex(value >> 4);
						str[j++] = asHex(value & 0x0F);
					}
					return str;
				}

				std::unique_ptr<char []> asPackedString() const noexcept
				{
					auto str = makeUnique<char []>(32);
					if (!str)
						return nullptr;
					for (uint8_t i{0}, j{0}; i < 16; ++i)
					{
						// This works because internally we keep things big endian
						const uint8_t value = asBuffer()[i];
						str[j++] = asHex(value >> 4);
						str[j++] = asHex(value & 0x0F);
					}
					return str;
				}
			};

			inline bool operator ==(const ormUUID_t &a, const ormUUID_t &b) noexcept { return a._uuid == b._uuid; }
		}
	}
}

#endif /*tmplORM_TYPES__HXX*/
