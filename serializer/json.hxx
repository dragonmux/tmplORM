#ifndef tmplORM_JSON__HXX
#define tmplORM_JSON__HXX

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

struct json_t
{
protected:
	std::unique_ptr<jsonAtom_t> rootAtom;

	json_t(stream_t &json) noexcept;
};

using rSON::typeIs;
using rSON::typeIsOrNull;

#endif /*tmplORM_JSON__HXX*/
