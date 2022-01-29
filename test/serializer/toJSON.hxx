#ifndef TO_JSON_HXX
#define TO_JSON_HXX

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
		data.add("categories", process<category_t>(session));
		data.add("suppliers", process<supplier_t>(session));
		data.add("products", process<product_t>(session));
		data.add("customers", process<customer_t>(session));
		data.add("shippers", process<shipper_t>(session));
		data.add("regions", process<region_t>(session));
		data.add("territories", process<territory_t>(session));
		data.add("employees", process<employee_t>(session));
		data.add("orders", process<order_t>(session));
		data.add("demographics", process<demographic_t>(session));
		data.add("customerDemographics", process<customerDemographic_t>(session));

		fileStream_t jsonFile{file, O_RDWR | O_NOCTTY | O_CREAT | O_TRUNC, S_IREAD | S_IWUSR};
		rSON::writeJSON(&data, jsonFile);
	}
}

#endif /*TO_JSON_HXX*/
