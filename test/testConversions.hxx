#ifndef TEST_CONVERSIONS__HXX
#define TEST_CONVERSIONS__HXX

#include <utility>
#include <vector>

template<typename int_t> using testOkPair_t = std::pair<int_t, const char *const>;

template<typename int_t> using testOk_t = std::vector<testOkPair_t<int_t>>;
template<typename int_t> using testFailInt_t = std::vector<int_t>;
using testFailStr_t = std::vector<const char *const>;

extern void testUint8(testsuit &suit, const testOk_t<uint8_t> &okTests, const testFailInt_t<uint8_t> &failTest);
extern void testInt8(testsuit &suit, const testOk_t<int8_t> &okTests, const testFailInt_t<int8_t> &failTest);

#endif /*TEST_CONVERSIONS__HXX*/
