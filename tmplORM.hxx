#ifndef tmplORM__HXX
#define tmplORM__HXX

#include <typestring/typestring.hh>

namespace tmplORM
{
	using namespace irqus;

	template<typename _tableName> struct model_t
	{
	};

	namespace types
	{
		template<typename _fieldName> struct field_t
		{
		};
	}
}

#endif /*tmplORM__HXX*/
