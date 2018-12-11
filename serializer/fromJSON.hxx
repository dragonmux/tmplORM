#ifndef tmplORM_FROM_JSON__HXX
#define tmplORM_FROM_JSON__HXX

#include <memory>
#include <tmplORM.hxx>
#include <fixedVector.hxx>
#include <models.hxx>
#include "json.hxx"
#include "helpers.hxx"

using namespace models;

struct fromJSON_t final : public json_t
{
private:
	bool validateProducts(const jsonAtom_t &productsAtom) const noexcept;
	bool validateCustomers(const jsonAtom_t &customersAtom) const noexcept;

public:
	fromJSON_t(stream_t &json) noexcept : json_t{json} { }
	fromJSON_t(const fromJSON_t &) = delete;
	fromJSON_t(fromJSON_t &&) = default;
	fromJSON_t &operator =(const fromJSON_t &) = delete;
	fromJSON_t &operator =(fromJSON_t &&) = default;

	bool validate() const noexcept;

	fixedVector_t<product_t> products() const noexcept;
	fixedVector_t<customer_t> customers() const noexcept;
};

namespace tmplORM
{
	namespace json
	{
		using tmplORM::types::type_t;
		using tmplORM::types::primary_t;
		using tmplORM::types::autoInc_t;
		using tmplORM::types::alias_t;
		using tmplORM::types::nullable_t;
		using tmplORM::types::unicode_t;
		using tmplORM::types::unicodeText_t;
		using tmplORM::types::date_t;
		using tmplORM::types::time_t;
		using tmplORM::types::dateTime_t;
		using tmplORM::types::uuid_t;
		using tmplORM::utils::lowerCamelCase_t;
		using tmplORM::utils::isInteger;
		using tmplORM::utils::enableIf;

		template<typename model_t> struct modelFromJSON_t
		{
			template<typename fieldName, typename T> static void populateField(model_t &model,
				const jsonObject_t &data, const alias_t<fieldName, T> &)
				{ model[fieldName{}] = data[lowerCamelCase_t<fieldName>::value]; }

			template<typename fieldName, typename T> static void populateField(model_t &model,
				const jsonObject_t &data, const type_t<fieldName, T> &)
				{ model[fieldName{}] = data[lowerCamelCase_t<fieldName>::value]; }

			template<typename field_t> static void populate(model_t &model, const jsonObject_t &data)
				{ populateField(model, data, field_t{}); }

			template<typename field_t, typename field__t, typename... fields_t> static void populate(
				model_t &model, const jsonObject_t &data)
			{
				populate<field_t>(model, data);
				populate<field__t, fields_t...>(model, data);
			}

			template<typename... fields_t> static model_t convert(const jsonObject_t &object)
			{
				model_t model{};
				populate<fields_t...>(model, object);
				return std::move(model);
			}
		};

		template<typename T, typename tableName, typename... fields_t> T modelFromJSON(const jsonObject_t &object,
			const model_t<tableName, fields_t...> &) { return modelFromJSON_t<T>::template convert<fields_t...>(object); }
	}

	template<typename T> T modelFromJSON(const jsonObject_t &data)
		{ return tmplORM::json::modelFromJSON<T>(data, T()); }
}

#endif /*tmplORM_FROM_JSON__HXX*/
