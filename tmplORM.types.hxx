#ifndef tmplORM_TYPES__HXX
#define tmplORM_TYPES__HXX

#include <conversions.hxx>
#include <cstring>

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
				int16_t _year;
				uint8_t _month;
				uint8_t _day;

				friend bool operator ==(const ormDate_t &a, const ormDate_t &b) noexcept;

			public:
				constexpr ormDate_t() noexcept : _year{0}, _month{0}, _day{0} { }
				constexpr ormDate_t(const int16_t year, const uint8_t month, const uint8_t day) noexcept :
					_year{year}, _month{month}, _day{day} { }

				constexpr int16_t year() const noexcept { return _year; }
				void year(const int16_t year) noexcept { _year = year; }
				constexpr uint8_t month() const noexcept { return _month; }
				void month(const uint8_t month) noexcept { _month = month; }
				constexpr uint8_t day() const noexcept { return _day; }
				void day(const uint8_t day) noexcept { _day = day; }

				ormDate_t(const char *date) noexcept : ormDate_t{}
				{
					const auto negative = *date == '-';
					_year = toInt_t<int16_t>{date, size_t(4 + negative)};
					date += 5 + negative;
					_month = toInt_t<uint8_t>{date, 2};
					date += 3;
					_day = toInt_t<uint8_t>{date, 2};
				}

				std::unique_ptr<char []> asString() const noexcept
				{
					const auto year = fromInt<4>(uint16_t(_year));
					const auto month = fromInt<2>(_month);
					const auto day = fromInt<2>(_day);

					auto str = makeUnique<char []>(year.length() + month.length() + day.length());
					if (!str)
						return nullptr;

					uint16_t offset = 0;
					year.formatTo(str.get() + offset);
					offset += year.length();
					str[offset - 1] = '-';
					month.formatTo(str.get() + offset);
					offset += month.length();
					str[offset - 1] = '-';
					day.formatTo(str.get() + offset);
					return str;
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
					// The strlen() check allows for 19 digits to follow. A uint64_t supports a maximum of
					// 19 digits of any value + a 20th 0-or-1 digit. This provides a good ballance between
					// support complexity and correctness.
					if (time[0] == 0 || std::strlen(time) > 20)
						return;
					++time;
					toInt_t<uint64_t> nanoSeconds(time);
					if (nanoSeconds.length() <= 9)
						_nanoSecond = nanoSeconds * power10(9 - nanoSeconds.length());
					else
						_nanoSecond = nanoSeconds / power10(nanoSeconds.length() - 9);
				}

				std::unique_ptr<char []> asString() const noexcept
				{
					const auto hour = fromInt<2>(_hour);
					const auto minute = fromInt<2>(_minute);
					const auto second = fromInt<2>(_second);
					fromInt_t<uint32_t, uint32_t> nanoSecond{_nanoSecond};

					auto str = makeUnique<char []>(hour.length() + minute.length() +
						second.length() + nanoSecond.fractionLength(9) + 1);
					if (!str)
						return nullptr;

					uint16_t offset = 0;
					hour.formatTo(str.get() + offset);
					offset += hour.length();
					str[offset - 1] = ':';
					minute.formatTo(str.get() + offset);
					offset += minute.length();
					str[offset - 1] = ':';
					second.formatTo(str.get() + offset);
					offset += second.length();
					str[offset - 1] = '.';
					nanoSecond.formatFractionTo(9, str.get() + offset);
					offset += nanoSecond.fractionLength(9);
					str[offset - 1] = 'Z';
					str[offset] = 0;
					return str;
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
				constexpr target_t durationAs(const source_t &d) noexcept
					{ return std::chrono::duration_cast<target_t>(d); }
				template<typename target_t, typename source_t>
				constexpr typename target_t::rep durationIn(const source_t &d) noexcept
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

				static std::array<std::array<uint16_t, 12>, 2> monthDays;

				constexpr bool isLeap(const rep_t year) noexcept
					{ return (year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0); }
				constexpr bool isLeap(const years &year) noexcept { return isLeap(year.count()); }
				constexpr rep_t div(const years a, const rep_t b) noexcept
					{ return (a.count() / b) + ((a.count() % b) < 0); }
				constexpr days leapsFor(const years year) noexcept
					{ return days{div(year, 4) - div(year, 100) + div(year, 400)}; }

				struct ormDateTime_t final : public ormDate_t, ormTime_t
				{
				private:
					struct timezone_t
					{
						::int32_t offset;
						::int32_t leapCorrection;
						size_t leapCount;
					};

					void display() const noexcept { printf("%04u-%02u-%02u %02u:%02u:%02u.%u\n", _year, _month, _day, _hour, _minute, _second, _nanoSecond); }

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

					int16_t computeYear(days &day) noexcept
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

					void tzComputeLeaps(timezone_t &result, const time_t timeSecs) noexcept;
					timezone_t tzCompute(const systemTime_t &time) noexcept;
					friend bool operator ==(const ormDateTime_t &a, const ormDateTime_t &b) noexcept;

				public:
					constexpr ormDateTime_t() noexcept : ormDate_t{}, ormTime_t{} { }
					constexpr ormDateTime_t(const int16_t year, const uint8_t month, const uint8_t day,
						const uint16_t hour, const uint16_t minute, const uint16_t second, const uint32_t nanoSecond) noexcept :
						ormDate_t{year, month, day}, ormTime_t{hour, minute, second, nanoSecond} { }
					ormDateTime_t(const char *dateTime) noexcept : ormDate_t{dateTime}, ormTime_t{dateTime + 11} { }

					ormDateTime_t(const systemTime_t time) noexcept : ormDateTime_t{}
					{
						const timezone_t timeZone = tzCompute(time);
						auto day = days{durationAs<seconds>(time) / seconds{1_day}};
						auto rem = time - day;
						rem += seconds{timeZone.offset - timeZone.leapCorrection};
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

					std::unique_ptr<char []> asString() const noexcept
					{
						const auto year = fromInt<4>(uint16_t(_year));
						const auto month = fromInt<2>(_month);
						const auto day = fromInt<2>(_day);
						const auto hour = fromInt<2>(_hour);
						const auto minute = fromInt<2>(_minute);
						const auto second = fromInt<2>(_second);
						fromInt_t<uint32_t, uint32_t> nanoSecond{_nanoSecond};

						auto str = makeUnique<char []>(year.length() + month.length() + day.length() + 1 +
							hour.length() + minute.length() + second.length() + nanoSecond.fractionLength(9));
						if (!str)
							return nullptr;

						uint16_t offset = 0;
						year.formatTo(str.get() + offset);
						offset += year.length();
						str[offset - 1] = '-';
						month.formatTo(str.get() + offset);
						offset += month.length();
						str[offset - 1] = '-';
						day.formatTo(str.get() + offset);
						offset += day.length();
						str[offset - 1] = 'T';
						hour.formatTo(str.get() + offset);
						offset += hour.length();
						str[offset - 1] = ':';
						minute.formatTo(str.get() + offset);
						offset += minute.length();
						str[offset - 1] = ':';
						second.formatTo(str.get() + offset);
						offset += second.length();
						str[offset - 1] = '.';
						nanoSecond.formatFractionTo(9, str.get() + offset);
						offset += nanoSecond.fractionLength(9);
						str[offset - 1] = 'Z';
						str[offset] = 0;
						return str;
					}
				};

				inline bool operator ==(const ormDateTime_t &a, const ormDateTime_t &b) noexcept
				{
					return static_cast<const ormDate_t &>(a) == static_cast<const ormDate_t &>(b) &&
						static_cast<const ormTime_t &>(a) == static_cast<const ormTime_t &>(b);
				}
			}

			using chrono::ormDateTime_t;
			using chrono::operator ==;

			inline bool operator !=(const ormDateTime_t &a, const ormDateTime_t &b) noexcept { return !(a == b); }
			//inline bool operator ==(const ormDate_t &a, const ormDateTime_t &b) noexcept { return a == ormDate_t(b); }
			inline bool operator ==(const ormDateTime_t &a, const ormDate_t &b) noexcept { return static_cast<const ormDate_t &>(a) == b; }
			inline bool operator ==(const ormDateTime_t &a, const ormTime_t &b) noexcept { return static_cast<const ormTime_t &>(a) == b; }

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
					const uint64_t e) noexcept : _uuid{a, b, c, uint64_t(e << 16U)}
				{
					swapBytes(_uuid.data1);
					swapBytes(_uuid.data2);
					swapBytes(_uuid.data3);
					_uuid.data4 |= swapBytes(d);
				}

				const guid_t *asPointer() const noexcept { return &_uuid; }
				guid_t *asPointer() noexcept { return &_uuid; }
				constexpr uint32_t data1() const noexcept { return _uuid.data1; }
				constexpr uint16_t data2() const noexcept { return _uuid.data2; }
				constexpr uint16_t data3() const noexcept { return _uuid.data3; }
				const uint8_t *data4() const noexcept
					{ return reinterpret_cast<const uint8_t *>(&_uuid.data4); } // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) lgtm [cpp/reinterpret-cast]

				ormUUID_t(const char *uuid) noexcept : ormUUID_t()
				{
					_uuid.data1 = toInt_t<uint32_t>(uuid, 8).fromHex();
					uuid += 9;
					_uuid.data2 = toInt_t<uint16_t>(uuid, 4).fromHex();
					uuid += 5;
					_uuid.data3 = toInt_t<uint16_t>(uuid, 4).fromHex();
					uuid += 5;
					_uuid.data4 = uint64_t(toInt_t<uint16_t>(uuid, 4).fromHex()) << 48U;
					uuid += 5;
					_uuid.data4 |= uint64_t(toInt_t<uint16_t>(uuid, 4).fromHex()) << 32U;
					_uuid.data4 |= toInt_t<uint32_t>(uuid + 4, 8).fromHex();
				}

				std::unique_ptr<char []> asString() const noexcept
				{
					std::array<uint8_t, sizeof(guid_t)> buffer{};
					auto str = makeUnique<char []>(36);
					if (!str)
						return nullptr;
					memcpy(buffer.data(), asPointer(), sizeof(guid_t));
					for (uint8_t i{0}, j{0}; i < 16; ++i)
					{
						// This works because internally we keep things big endian
						const uint8_t value = buffer[i];
						if (i == 4 || i == 6 || i == 8 || i == 10)
							str[j++] = '-';
						str[j++] = asHex(value >> 4U);
						str[j++] = asHex(value & 0x0FU);
					}
					return str;
				}

				std::unique_ptr<char []> asPackedString() const noexcept
				{
					std::array<uint8_t, sizeof(guid_t)> buffer{};
					auto str = makeUnique<char []>(32);
					if (!str)
						return nullptr;
					memcpy(buffer.data(), asPointer(), sizeof(guid_t));
					for (uint8_t i{0}, j{0}; i < 16; ++i)
					{
						// This works because internally we keep things big endian
						const uint8_t value = buffer[i];
						str[j++] = asHex(value >> 4U);
						str[j++] = asHex(value & 0x0FU);
					}
					return str;
				}
			};

			inline bool operator ==(const ormUUID_t &a, const ormUUID_t &b) noexcept { return a._uuid == b._uuid; }
		}
	}
}

#endif /*tmplORM_TYPES__HXX*/
