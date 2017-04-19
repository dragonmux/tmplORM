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

class testValid final : public testsuit
{
private:
	void assertInvalid(const utf16_t &val)
	{
		const char16_t *const value = val;
		assertNull(value);
		assertTrue(!val);
	}

	void assertInvalid(const utf8_t &val)
	{
		const char *const value = val;
		assertNull(value);
		assertTrue(!val);
	}

public:
	void testBadUTF8()
	{
		assertInvalid(utf16::convert("\x80"));
		assertInvalid(utf16::convert("\xC0"));
		assertInvalid(utf16::convert("\xC0\x0A"));
		assertInvalid(utf16::convert("\xE0\x0A"));
		assertInvalid(utf16::convert("\xE0\x8A\x0A"));
		// This encodes part of a surrogate pair character - which is invalid in UTF-8.
		assertInvalid(utf16::convert("\xED\xAA\x8A"));
		assertInvalid(utf16::convert("\xF0\x0A"));
		assertInvalid(utf16::convert("\xF0\x8A\x0A"));
		assertInvalid(utf16::convert("\xF0\x8A\x8A\x0A"));
		// TODO: need to test full surrogate-pair value is not present as this is also invalid.
	}

	void testBadUTF16()
	{
		assertInvalid(utf16::convert(u"\xDC00"));
		assertInvalid(utf16::convert(u"\xD800"));
		assertInvalid(utf16::convert(u"\xD800\x08A0"));
	}

	void registerTests() final override
	{
		CXX_TEST(testBadUTF8)
		CXX_TEST(testBadUTF16)
	}
};

class testFormat final : public testsuit
{
public:
	void testDup()
	{
		auto str = strNewDup("This is only a test");
		assertNotNull(str.get());
		assertEqual(str.get(), "This is only a test");
	}

	void registerTests() final override
	{
		CXX_TEST(testDup)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testConvert, testValid, testFormat>();
}

