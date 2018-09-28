#ifndef tmplORM_TO_JSON__HXX
#define tmplORM_TO_JSON__HXX

#include <memory>
#include <tmplORM.hxx>
#include <fixedVector.hxx>
#include "json.hxx"

namespace tmplORM
{
	namespace json
	{
		using tmplORM::types::type_t;
		using tmplORM::types::alias_t;
		using tmplORM::types::nullable_t;
		using tmplORM::types::unicode_t;
		using tmplORM::types::unicodeText_t;
		using tmplORM::utils::lowerCamelCase_t;

		template<typename> struct isBoolean : public std::false_type { };
		template<> struct isBoolean<bool> : public std::true_type { };
		template<typename T> using isIntegral = std::is_integral<T>;
		template<typename T> using isInteger = std::integral_constant<bool,
			!isBoolean<T>::value && isIntegral<T>::value>;
		template<typename T> using isFloatingPoint = std::is_floating_point<T>;
		template<bool B, typename T = void> using enableIf = typename std::enable_if<B, T>::type;

		template<typename> struct makeAtom_t;
		template<typename field_t, typename value_t> std::unique_ptr<jsonAtom_t> makeAtom(const value_t &value)
			{ return makeAtom_t<field_t>::from(value); }

		template<typename T> struct makeAtom_t<nullable_t<T>>
		{
			static std::unique_ptr<jsonAtom_t> from(const nullable_t<T> &value)
			{
				if (value.isNull())
					return makeUnique<jsonNull_t>();
				return makeAtom_t<T>::from(value);
			}
		};

		template<typename fieldName, typename T> struct makeAtom_t<type_t<fieldName, T>>
		{
			static std::unique_ptr<jsonBool_t> from(const type_t<fieldName, bool> &value)
				{ return makeUnique<jsonBool_t>(value.value()); }

			static std::unique_ptr<jsonInt_t> from(const type_t<fieldName, enableIf<isInteger<T>::value, T>> &value)
				{ return makeUnique<jsonInt_t>(value.value()); }

			static std::unique_ptr<jsonFloat_t> from(const type_t<fieldName,
				enableIf<isFloatingPoint<T>::value, T>> &value) { return makeUnique<jsonFloat_t>(value.value()); }
		};

		template<typename fieldName, size_t N> struct makeAtom_t<unicode_t<fieldName, N>>
		{
			static std::unique_ptr<jsonString_t> from(const unicode_t<fieldName, N> &value)
				{ return makeUnique<jsonString_t>(value.value()); }
		};

		template<typename model_t> struct modelToJSON_t
		{
			template<typename field_t, typename fieldName, typename T> static void populateField(
				const model_t &model, jsonObject_t &data, const alias_t<fieldName, T> &)
				{ data.add(lowerCamelCase_t<fieldName>::value, makeAtom<field_t>(model[fieldName()]).release()); }

			template<typename field_t, typename fieldName, typename T> static void populateField(
				const model_t &model, jsonObject_t &data, const type_t<fieldName, T> &)
				{ data.add(lowerCamelCase_t<fieldName>::value, makeAtom<field_t>(model[fieldName()]).release()); }

			template<typename field_t> static void populate(const model_t &model, jsonObject_t &data)
				{ populateField<field_t>(model, data, field_t()); }

			template<typename field_t, typename field__t, typename... fields_t> static void populate(
				const model_t &model, jsonObject_t &data)
			{
				populate<field_t>(model, data);
				populate<field__t, fields_t...>(model, data);
			}

			template<typename... fields_t> static void convert(const model_t &model, jsonObject_t &object)
				{ populate<fields_t...>(model, object); }
		};

		template<typename tableName, typename... fields_t> std::unique_ptr<jsonAtom_t>
			modelToJSON(const model_t<tableName, fields_t...> &model)
		{
			std::unique_ptr<jsonAtom_t> data = makeUnique<jsonObject_t>();
			modelToJSON_t<model_t<tableName, fields_t...>>::template convert<fields_t...>(model, *data);
			return data;
		}
	}

	template<typename T> std::unique_ptr<jsonAtom_t> modelToJSON(const T &model)
		{ return tmplORM::json::modelToJSON(model); }
}

#endif /*tmplORM_TO_JSON__HXX*/
