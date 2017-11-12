#ifndef TEST_CONVERSIONS__HXX
#define TEST_CONVERSIONS__HXX

#include <utility>
#include <vector>

template<typename int_t> using testOkPair_t = std::pair<int_t, const char *const>;

template<typename int_t> using testOk_t = std::vector<testOkPair_t<int_t>>;
template<typename int_t> using testFailInt_t = std::vector<int_t>;
using testFailStr_t = std::vector<const char *>;

extern void testFromUint8(testsuit &suite, const testOk_t<uint8_t> tests);
extern void testFromInt8(testsuit &suite, const testOk_t<int8_t> tests);
extern void testFromUint16(testsuit &suite, const testOk_t<uint16_t> tests);
extern void testFromInt16(testsuit &suite, const testOk_t<int16_t> tests);
extern void testFromUint32(testsuit &suite, const testOk_t<uint32_t> tests);
extern void testFromInt32(testsuit &suite, const testOk_t<int32_t> tests);
extern void testFromUint64(testsuit &suite, const testOk_t<uint64_t> tests);
extern void testFromInt64(testsuit &suite, const testOk_t<int64_t> tests);

extern void testDecToUint8(testsuit &suite, const testOk_t<uint8_t> tests);
extern void testDecToInt8(testsuit &suite, const testOk_t<int8_t> tests);
extern void testDecToUint16(testsuit &suite, const testOk_t<uint16_t> tests);
extern void testDecToInt16(testsuit &suite, const testOk_t<int16_t> tests);
extern void testDecToUint32(testsuit &suite, const testOk_t<uint32_t> tests);
extern void testDecToInt32(testsuit &suite, const testOk_t<int32_t> tests);
extern void testDecToUint64(testsuit &suite, const testOk_t<uint64_t> tests);
extern void testDecToInt64(testsuit &suite, const testOk_t<int64_t> tests);
extern void testDecShouldFail(testsuit &suite, const testFailStr_t tests);

extern void testHexToUint8(testsuit &suite, const testOk_t<uint8_t> tests);
extern void testHexToInt8(testsuit &suite, const testOk_t<int8_t> tests);
extern void testHexToUint16(testsuit &suite, const testOk_t<uint16_t> tests);
extern void testHexToInt16(testsuit &suite, const testOk_t<int16_t> tests);
extern void testHexToUint32(testsuit &suite, const testOk_t<uint32_t> tests);
extern void testHexToInt32(testsuit &suite, const testOk_t<int32_t> tests);
extern void testHexToUint64(testsuit &suite, const testOk_t<uint64_t> tests);
extern void testHexToInt64(testsuit &suite, const testOk_t<int64_t> tests);
extern void testHexShouldFail(testsuit &suite, const testFailStr_t tests);

extern uint16_t testSwapBytes(const uint16_t val) noexcept;
extern uint32_t testSwapBytes(const uint32_t val) noexcept;
extern uint64_t testSwapBytes(const uint64_t val) noexcept;

#endif /*TEST_CONVERSIONS__HXX*/
