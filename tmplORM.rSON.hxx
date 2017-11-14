#ifndef tmplORM_rSON__HXX
#define tmplORM_rSON__HXX

#include <rSON.h>
#include <tmplORM/tmplORM.hxx>

namespace tmplORM
{
	namespace types
	{
		namespace rSON
		{
			using jsonAtom_t = rSON::JSONAtom;

			template<typename nullable_t, bool = std::is_same<typename std::remove_const<typename nullable_t::type>::type, char *>::value ||
				std::is_same<typename nullable_t::type, types::baseTypes::ormDateTime_t>::value> ||
				std::is_same<typename nullable_t::type, types::baseTypes::ormDate_t>::value>
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
						// TODO: This is madness.. a copy in this case is not insane.
						container.value(const_cast<char *>(rawValue));
					}
				}
			};

			template<typename T> struct jsonNullable_t : public types::nullable_t<T>
			{
			private:
				using nullable_t = types::nullable_t<T>;
		
			public:
				using type = typename nullable_t::type;
				using nullable_t::operator =;
				using nullable_t::value;
				using nullable_t::operator ==;
				using nullable_t::operator !=;
		
				void operator =(const jsonAtom_t &_value) noexcept { value(_value); }
				void value(const jsonAtom_t &_value)
				{
					const nullableValue_t<nullable_t> setter;
					setter(*this, _value);
				}
			};
		}
	}
}

#endif /*tmplORM_rSON__HXX*/
