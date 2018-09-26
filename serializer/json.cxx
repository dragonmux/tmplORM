#include "internal.hxx"

using namespace rSON;

json_t::json_t(stream_t &json) noexcept : rootAtom(nullptr)
{
	try
		{ rootAtom.reset(parseJSON(json)); }
	catch (JSONParserError &error)
		{ logerr("Error parsing JSON: %s", error.error()); }
	catch (std::bad_alloc &)
		{ logerr("Failed to allocate memory"); }
}
