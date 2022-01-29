#ifndef TEST_CONVERSIONS__HXX
#define TEST_CONVERSIONS__HXX

#include <utility>
#include <vector>

template<typename int_t> using testOkPair_t = std::pair<int_t, const char *const>;

template<typename int_t> using testOk_t = std::vector<testOkPair_t<int_t>>;
template<typename int_t> using testFailInt_t = std::vector<int_t>;
using testFailStr_t = std::vector<const char *>;

extern void testFromUint8(testsuite &suite, const testOk_t<uint8_t> tests);
extern void testFromInt8(testsuite &suite, const testOk_t<int8_t> tests);
extern void testFromUint16(testsuite &suite, const testOk_t<uint16_t> tests);
extern void testFromInt16(testsuite &suite, const testOk_t<int16_t> tests);
extern void testFromUint32(testsuite &suite, const testOk_t<uint32_t> tests);
extern void testFromInt32(testsuite &suite, const testOk_t<int32_t> tests);
extern void testFromUint64(testsuite &suite, const testOk_t<uint64_t> tests);
extern void testFromInt64(testsuite &suite, const testOk_t<int64_t> tests);

extern void testOctToUint8(testsuite &suite, const testOk_t<uint8_t> tests);
extern void testOctToInt8(testsuite &suite, const testOk_t<int8_t> tests);
extern void testOctToUint16(testsuite &suite, const testOk_t<uint16_t> tests);
extern void testOctToInt16(testsuite &suite, const testOk_t<int16_t> tests);
extern void testOctToUint32(testsuite &suite, const testOk_t<uint32_t> tests);
extern void testOctToInt32(testsuite &suite, const testOk_t<int32_t> tests);
extern void testOctToUint64(testsuite &suite, const testOk_t<uint64_t> tests);
extern void testOctToInt64(testsuite &suite, const testOk_t<int64_t> tests);
extern void testOctShouldFail(testsuite &suite, const testFailStr_t tests);

extern void testDecToUint8(testsuite &suite, const testOk_t<uint8_t> tests);
extern void testDecToInt8(testsuite &suite, const testOk_t<int8_t> tests);
extern void testDecToUint16(testsuite &suite, const testOk_t<uint16_t> tests);
extern void testDecToInt16(testsuite &suite, const testOk_t<int16_t> tests);
extern void testDecToUint32(testsuite &suite, const testOk_t<uint32_t> tests);
extern void testDecToInt32(testsuite &suite, const testOk_t<int32_t> tests);
extern void testDecToUint64(testsuite &suite, const testOk_t<uint64_t> tests);
extern void testDecToInt64(testsuite &suite, const testOk_t<int64_t> tests);
extern void testDecShouldFail(testsuite &suite, const testFailStr_t tests);

extern void testHexToUint8(testsuite &suite, const testOk_t<uint8_t> tests);
extern void testHexToInt8(testsuite &suite, const testOk_t<int8_t> tests);
extern void testHexToUint16(testsuite &suite, const testOk_t<uint16_t> tests);
extern void testHexToInt16(testsuite &suite, const testOk_t<int16_t> tests);
extern void testHexToUint32(testsuite &suite, const testOk_t<uint32_t> tests);
extern void testHexToInt32(testsuite &suite, const testOk_t<int32_t> tests);
extern void testHexToUint64(testsuite &suite, const testOk_t<uint64_t> tests);
extern void testHexToInt64(testsuite &suite, const testOk_t<int64_t> tests);
extern void testHexShouldFail(testsuite &suite, const testFailStr_t tests);

#endif /*TEST_CONVERSIONS__HXX*/
