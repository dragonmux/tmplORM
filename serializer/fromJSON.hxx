#ifndef tmplORM_FROM_JSON__HXX
#define tmplORM_FROM_JSON__HXX

#include <memory>
#include <tmplORM.hxx>
#include <substrate/fixed_vector>
#include "json.hxx"
#include "helpers.hxx"

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

		template<typename T> fixedVector_t<T> modelArrayFromJSONArr(const jsonArray_t &jsonData)
		{
			fixedVector_t<T> dbData(jsonData.count());
			if (!dbData.valid())
				return {};
			for (size_t i = 0; i < jsonData.count(); ++i)
				dbData[i] = modelFromJSON<T>(jsonData[i], T{});
			return dbData;
		}

		template<typename T, typename tableName, typename... fields_t>
			fixedVector_t<T> modelArrayFromJSONObj(const jsonObject_t &data, const model_t<tableName, fields_t...> &)
			{ return modelArrayFromJSONArr<T>(data[lowerCamelCase_t<tableName>::value]); }

		template<typename> struct atomType_t {};
		template<> struct atomType_t<int8_t> { constexpr static jsonAtomType_t value = rSON::JSON_TYPE_INT; };
		template<> struct atomType_t<uint8_t> { constexpr static jsonAtomType_t value = rSON::JSON_TYPE_INT; };
		template<> struct atomType_t<int16_t> { constexpr static jsonAtomType_t value = rSON::JSON_TYPE_INT; };
		template<> struct atomType_t<uint16_t> { constexpr static jsonAtomType_t value = rSON::JSON_TYPE_INT; };
		template<> struct atomType_t<int32_t> { constexpr static jsonAtomType_t value = rSON::JSON_TYPE_INT; };
		template<> struct atomType_t<uint32_t> { constexpr static jsonAtomType_t value = rSON::JSON_TYPE_INT; };
		template<> struct atomType_t<int64_t> { constexpr static jsonAtomType_t value = rSON::JSON_TYPE_INT; };
		template<> struct atomType_t<uint64_t> { constexpr static jsonAtomType_t value = rSON::JSON_TYPE_INT; };
		template<> struct atomType_t<bool> { constexpr static jsonAtomType_t value = rSON::JSON_TYPE_BOOL; };
		template<> struct atomType_t<float> { constexpr static jsonAtomType_t value = rSON::JSON_TYPE_FLOAT; };
		template<> struct atomType_t<double> { constexpr static jsonAtomType_t value = rSON::JSON_TYPE_FLOAT; };
		template<> struct atomType_t<void *> { constexpr static jsonAtomType_t value = rSON::JSON_TYPE_STRING; };

		template<typename T> static bool typeIs(const jsonAtom_t &value) noexcept { return rSON::typeIs<T>(value); }

		template<typename field_t> struct dataType_t;
		template<typename field_t> struct validatePrimary_t
		{
			template<typename T> static bool unwrap(const jsonAtom_t &value, const autoInc_t<T> &) noexcept
				{ return unwrap(value, T{}); }
			template<typename fieldName, typename T, typename = enableIf<isInteger<T>::value>>
				static bool unwrap(const jsonAtom_t &value, const type_t<fieldName, T> &) noexcept
				{ return dataType_t<type_t<fieldName, T>>::validate(value) && validateID(value); }
			template<typename T> static bool unwrap(const jsonAtom_t &value, const T &) noexcept
				{ return dataType_t<T>::validate(value); }

			static bool check(const jsonAtom_t &value) noexcept
				{ return unwrap(value, field_t{}); }
		};

		template<typename field_t> struct dataType_t
		{
			template<typename T> static bool validate_(const jsonAtom_t &value, const nullable_t<T> &) noexcept
				{ return typeIs<rSON::JSON_TYPE_NULL>(value) || dataType_t<T>::validate(value); }
			template<typename T> static bool validate_(const jsonAtom_t &value, const primary_t<T> &) noexcept
				{ return validatePrimary_t<T>::check(value); }
			template<typename fieldName> static bool validate_(const jsonAtom_t &value,
				const date_t<fieldName> &) noexcept
				{ return typeIs<rSON::JSON_TYPE_STRING>(value) && validateDate(value); }
			template<typename fieldName> static bool validate_(const jsonAtom_t &value,
				const dateTime_t<fieldName> &) noexcept
				{ return typeIs<rSON::JSON_TYPE_STRING>(value) && validateDateTime(value); }
			template<typename fieldName> static bool validate_(const jsonAtom_t &value,
				const uuid_t<fieldName> &) noexcept
				{ return typeIs<rSON::JSON_TYPE_STRING>(value) && validateUUID(value); }
			template<typename fieldName, typename T> static bool validate_(const jsonAtom_t &value,
				const type_t<fieldName, T> &) noexcept { return typeIs<atomType_t<T>::value>(value); }
			template<typename fieldName> static bool validate_(const jsonAtom_t &value,
				const unicodeText_t<fieldName> &) noexcept { return typeIs<rSON::JSON_TYPE_STRING>(value); }
			template<typename fieldName, size_t length> static bool validate_(const jsonAtom_t &value,
				const unicode_t<fieldName, length> &) noexcept
				{ return typeIs<rSON::JSON_TYPE_STRING>(value) && value.asStringRef().len() <= length; }

			static bool validate(const jsonAtom_t &value) noexcept { return validate_(value, field_t{}); }
		};

		template<typename field_t, typename fieldName, typename T> bool validateField(const jsonObject_t &data,
			const type_t<fieldName, T> &) noexcept
		{
			using name = lowerCamelCase_t<fieldName>;
			if (!data.exists(name::value))
				return false;
			const jsonAtom_t &field = data[name::value];
			return dataType_t<field_t>::validate(field);
		}

		template<typename field_t> bool validate(const jsonObject_t &object) noexcept
			{ return validateField<field_t>(object, field_t{}); }
		template<typename field_t, typename field__t, typename... fields_t>
			bool validate(const jsonObject_t &object) noexcept
			{ return validate<field_t>(object) && validate<field__t, fields_t...>(object); }
		template<typename tableName, typename... fields_t> bool isValidJSON(const jsonObject_t &object,
			const model_t<tableName, fields_t...> &) noexcept { return validate<fields_t...>(object); }

		template<typename T> bool isValidJSONArray(const jsonAtom_t &atom) noexcept
		{
			if (!typeIs<rSON::JSON_TYPE_ARRAY>(atom))
				return false;
			for (const auto &data : atom.asArrayRef())
			{
				if (!typeIs<rSON::JSON_TYPE_OBJECT>(*data) ||
					!isValidJSON(*data, T{}))
					return false;
			}
			return true;
		}
	}

	using tmplORM::json::jsonAtom_t;
	using tmplORM::json::jsonObject_t;
	using tmplORM::json::jsonArray_t;

	template<typename T> T modelFromJSON(const jsonObject_t &data)
		{ return tmplORM::json::modelFromJSON<T>(data, T{}); }
	template<typename T> fixedVector_t<T> modelArrayFromJSONArr(const jsonArray_t &data)
		{ return tmplORM::json::modelArrayFromJSONArr<T>(data); }
	template<typename T> fixedVector_t<T> modelArrayFromJSONObj(const jsonObject_t &data)
		{ return tmplORM::json::modelArrayFromJSONObj<T>(data, T{}); }
	template<typename T> bool isValidJSON(const jsonObject_t &data)
		{ return tmplORM::json::isValidJSON(data, T{}); }
	template<typename T> bool isValidJSONArray(const jsonAtom_t &atom) noexcept
		{ return tmplORM::json::isValidJSONArray<T>(atom); }
}

#endif /*tmplORM_FROM_JSON__HXX*/
