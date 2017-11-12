#ifndef TEST_CONVERSIONS__HXX
#define TEST_CONVERSIONS__HXX

#include <utility>
#include <vector>

template<typename int_t> using testOkPair_t = std::pair<int_t, const char *const>;

template<typename int_t> using testOk_t = std::vector<testOkPair_t<int_t>>;
template<typename int_t> using testFailInt_t = std::vector<int_t>;
using testFailStr_t = std::vector<const char *const>;

extern void testUint8(testsuit &suit, const testOk_t<uint8_t> tests);
extern void testInt8(testsuit &suit, const testOk_t<int8_t> tests);
extern void testUint16(testsuit &suit, const testOk_t<uint16_t> tests);
extern void testInt16(testsuit &suit, const testOk_t<int16_t> tests);
extern void testUint32(testsuit &suit, const testOk_t<uint32_t> tests);
extern void testInt32(testsuit &suit, const testOk_t<int32_t> tests);
extern void testUint64(testsuit &suit, const testOk_t<uint64_t> tests);
extern void testInt64(testsuit &suit, const testOk_t<int64_t> tests);

#endif /*TEST_CONVERSIONS__HXX*/
