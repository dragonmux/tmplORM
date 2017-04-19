#include <crunch++.h>
#include <string.hxx>

class testConvert final : public testsuit
{
private:
	const char *const utf8Ref = u8"\u005B\u00D8\u04D5\u16A0\u2026\uFFFD\U00010117";
	const char16_t *const utf16Ref = u"\u005B\u00D8\u04D5\u16A0\u2026\uFFFD\U00010117";

public:
	void testValidity()
	{
		assertTrue(!utf8_t(nullptr));
		assertTrue(!utf16_t(nullptr));
	}

	void testUTF8to16()
	{
		utf16_t utf16Data = utf16::convert(utf8Ref);
		assertTrue(bool(utf16Data));
		const char16_t *const utf16Res = utf16Data;
		assertEqual(utf16Res, utf16Ref, utf16::length(utf16Ref));
	}

	void testUTF16to8()
	{
		utf8_t utf8Data = utf16::convert(utf16Ref);
		assertTrue(bool(utf8Data));
		const char *const utf8Res = utf8Data;
		assertEqual(utf8Res, utf8Ref, utf16::length(utf8Ref));
	}

	void registerTests() final override
	{
		CXX_TEST(testValidity)
		CXX_TEST(testUTF8to16)
		CXX_TEST(testUTF16to8)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testConvert>();
}

