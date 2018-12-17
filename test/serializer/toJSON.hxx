#ifndef TO_JSON__HXX
#define TO_JSON__HXX

#include "../models.hxx"
#include <serializer/toJSON.hxx>
#include <fcntl.h>
#ifndef _MSC_VER
#include <unistd.h>
#else
#include <io.h>
#endif

inline namespace toJSON
{
	using namespace models;
	using tmplORM::json::jsonAtom_t;
	using tmplORM::json::jsonObject_t;
	using rSON::fileStream_t;
	constexpr static int32_t S_IREAD = S_IRUSR | S_IRGRP | S_IROTH;

	template<typename model_t, typename session_t> std::unique_ptr<jsonAtom_t> process(session_t &session)
		{ return tmplORM::modelToJSON(session.template select<model_t>()); }

	template<typename session_t> void writeDataTo(const char *const file, session_t &session)
	{
		jsonObject_t data{};
		data.add("customers", process<customer_t>(session));

		fileStream_t jsonFile{file, O_RDWR | O_NOCTTY | O_CREAT | O_TRUNC, S_IREAD | S_IWUSR};
		rSON::writeJSON(&data, jsonFile);
	}
}

#endif /*TO_JSON__HXX*/
