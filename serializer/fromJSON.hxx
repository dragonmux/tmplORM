#ifndef FROM_JSON__HXX
#define FROM_JSON__HXX

#include <rSON.h>
#include <memory>
#include <fixedVector.hxx>
#include <models.hxx>

////
using rSON::stream_t;
using jsonAtom_t = rSON::JSONAtom;
using jsonObject_t = rSON::JSONObject;
using jsonArray_t = rSON::JSONArray;
using jsonString_t = rSON::JSONString;

struct json_t
{
protected:
	std::unique_ptr<jsonAtom_t> rootAtom;

	json_t(stream_t &json) noexcept;
};

using rSON::typeIs;
using rSON::typeIsOrNull;
////

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

#endif /*FROM_JSON__HXX*/
