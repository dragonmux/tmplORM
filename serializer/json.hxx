#ifndef tmplORM_JSON__HXX
#define tmplORM_JSON__HXX

#include <tmplORM.hxx>
#include <rSON.h>

using rSON::stream_t;
using jsonAtom_t = rSON::JSONAtom;
using jsonObject_t = rSON::JSONObject;
using jsonArray_t = rSON::JSONArray;
using jsonString_t = rSON::JSONString;
using jsonNull_t = rSON::JSONNull;
using jsonBool_t = rSON::JSONBool;
using jsonInt_t = rSON::JSONInt;
using jsonFloat_t = rSON::JSONFloat;
using jsonString_t = rSON::JSONString;
using jsonAtomType_t = rSON::JSONAtomType;

struct json_t
{
protected:
	std::unique_ptr<jsonAtom_t> rootAtom;

	json_t(stream_t &json) noexcept;
};

using rSON::typeIs;
using rSON::typeIsOrNull;

namespace tmplORM
{
	namespace types
	{
		namespace json
		{
			template<typename nullable_t, bool =
				std::is_same<typename std::remove_const<typename nullable_t::type>::type, char *>::value ||
				std::is_same<typename nullable_t::type, tmplORM::types::ormDateTime_t>::value ||
				std::is_same<typename nullable_t::type, tmplORM::types::ormDate_t>::value>
				struct nullableValue_t
			{
				using type = typename nullable_t::type;
				void operator ()(nullable_t &container, const jsonAtom_t &_value) const
				{
					if (_value.isNull())
						container.value(nullptr);
					else
					{
						const type rawValue = _value;
						container.value(rawValue);
					}
				}
			};

			template<typename nullable_t> struct nullableValue_t<nullable_t, true>
			{
				using type = typename nullable_t::type;
				void operator ()(nullable_t &container, const jsonAtom_t &_value) const
				{
					if (_value.isNull())
						container.value(nullptr);
					else
					{
						const char *const rawValue = _value;
						container.value(const_cast<char *>(rawValue));
					}
				}
			};
		}

		template<typename T> struct jsonNullable_t : public nullable_t<T>
		{
		private:
			using nullableValue_t = tmplORM::types::json::nullableValue_t<nullable_t<T>>;

		public:
			using type = typename nullable_t<T>::type;
			using nullable_t<T>::operator =;
			using nullable_t<T>::value;
			using nullable_t<T>::operator ==;
			using nullable_t<T>::operator !=;

			void operator =(const jsonAtom_t &_value) noexcept { value(_value); }
			void value(const jsonAtom_t &_value)
			{
				const nullableValue_t setter{};
				setter(*this, _value);
			}
		};
	}
}

#endif /*tmplORM_JSON__HXX*/
