#ifndef tmplORM_FROM_JSON__HXX
#define tmplORM_FROM_JSON__HXX

#include <memory>
#include <tmplORM.hxx>
#include <fixedVector.hxx>
#include <models.hxx>
#include "json.hxx"

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
		using tmplORM::types::alias_t;
		using tmplORM::utils::lowerCamelCase_t;

		template<typename...> struct populateFromJSON_t;
		template<> struct populateFromJSON_t<>
			{ template<typename T> static void populate(T &, const jsonObject_t &) { } };

		template<typename field_t, typename... fields_t> struct populateFromJSON_t<field_t, fields_t...>
		{
			template<typename model_t, typename fieldName, typename T> static void populateField(model_t &model,
				const jsonObject_t &data, const alias_t<fieldName, T> &)
				{ model[fieldName()] = data[lowerCamelCase<fieldName>::value]; }

			template<typename model_t, typename fieldName, typename T> static void populateField(model_t &model,
				const jsonObject_t &data, const type_t<fieldName, T> &)
				{ model[fieldName()] = data[lowerCamelCase<fieldName>::value]; }

			template<typename T> static void populate(T &model, const jsonObject_t &data)
			{
				populateField(model, data, field_t());
				populateFromJSON_t<fields_t...>::populate(model, data);
			}
		};

		template<typename T, typename... fields_t> struct modelFromJSON_t
		{
			static T convert(const jsonObject_t &object)
			{
				T model{};
				populateFromJSON_t<fields_t...>::populate(model, object);
				return std::move(model);
			}
		};

		template<typename T, typename tableName, typename... fields_t> T modelFromJSON(const jsonObject_t &object,
			const model_t<tableName, fields_t...> &) { return modelFromJSON_t<T, fields_t...>::convert(object); }
	}

	template<typename T> T modelFromJSON(const jsonObject_t &data)
		{ return tmplORM::json::modelFromJSON<T>(data, T()); }
}

#endif /*tmplORM_FROM_JSON__HXX*/
