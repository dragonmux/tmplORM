#include "fromJSON.hxx"
#include "toJSON.hxx"
#include <models.hxx>

using namespace models;

struct fromJSON_t final : public json_t
{
private:
	bool validateProducts(const jsonAtom_t &productsAtom) const noexcept;
	bool validateCustomers(const jsonAtom_t &customersAtom) const noexcept;
	bool validateEmployees(const jsonAtom_t &employeesAtom) const noexcept;

public:
	fromJSON_t(stream_t &json) noexcept : json_t{json} { }
	fromJSON_t(const fromJSON_t &) = delete;
	fromJSON_t(fromJSON_t &&) = default;
	fromJSON_t &operator =(const fromJSON_t &) = delete;
	fromJSON_t &operator =(fromJSON_t &&) = default;

	bool validate() const noexcept;

	fixedVector_t<product_t> products() const noexcept;
	fixedVector_t<customer_t> customers() const noexcept;
	fixedVector_t<employee_t> employees() const noexcept;
};

using namespace rSON;
using tmplORM::modelArrayFromJSONObj;

bool isIn(const char *const value, const char *const _value) noexcept { return strcmp(value, _value) == 0; }
template<typename... Values> bool isIn(const char *const value, const char *const _value, Values ...values) noexcept
{
	if (isIn(value, _value))
		return true;
	return isIn(value, values...);
}

bool fromJSON_t::validate() const noexcept
{
	if (!rootAtom || rootAtom->getType() != JSON_TYPE_OBJECT)
		return false;

	jsonObject_t &data = *rootAtom;
	for (const auto key : data.keys())
	{
		if (!isIn(key, "products", "customers", "employees"))
			return false;
	}

	// Validate primary (required) keys
	// if (!data.exists("key1") || !validateKey1(data["key1"]) ||
	//	!data.exists("key2") || !validateKey2(data["key2"]))

	// Validate secondary (optional) keys
   	if ((data.exists("products") && !validateProducts(data["products"])) ||
		(data.exists("customers") && !validateCustomers(data["customers"])) ||
		(data.exists("employees") && !validateEmployees(data["employees"])))
   		return false;

   	return true;
}

bool fromJSON_t::validateProducts(const jsonAtom_t &productsAtom) const noexcept
{
	if (!typeIs<JSON_TYPE_ARRAY>(productsAtom))
		return false;
	const jsonArray_t &products = productsAtom;
	for (const auto &productAtom : products)
	{
		if (!typeIs<JSON_TYPE_OBJECT>(*productAtom))
			return false;
		const jsonObject_t &product = *productAtom;
		if (!isValidJSON<product_t>(product) ||
			!validateIfInt<validateID>(product["supplierID"]) ||
			!validateIfInt<validateID>(product["categoryID"]))
			return false;
	}
	return true;
}

fixedVector_t<product_t> fromJSON_t::products() const noexcept
	{ return modelArrayFromJSONObj<product_t>(*rootAtom); }

bool fromJSON_t::validateCustomers(const jsonAtom_t &customersAtom) const noexcept
	{ return isValidJSONArray<customer_t>(customersAtom); }

fixedVector_t<customer_t> fromJSON_t::customers() const noexcept
	{ return modelArrayFromJSONObj<customer_t>(*rootAtom); }

void customers(fixedVector_t<customer_t> &dbCustomers)
{
	jsonArray_t jsonCustomers{};
	for (const auto &dbCustomer : dbCustomers)
		jsonCustomers.add(modelToJSON(dbCustomer).release());
}

bool fromJSON_t::validateEmployees(const jsonAtom_t &employeesAtom) const noexcept
{
	if (!typeIs<JSON_TYPE_ARRAY>(employeesAtom))
		return false;
	const jsonArray_t &employees = employeesAtom;
	for (const auto &employeeAtom : employees)
	{
		if (!typeIs<JSON_TYPE_OBJECT>(*employeeAtom))
			return false;
		const jsonObject_t &employee = *employeeAtom;
		if (!isValidJSON<employee_t>(employee))
			return false;
	}
	return true;
}

fixedVector_t<employee_t> fromJSON_t::employees() const noexcept
	{ return modelArrayFromJSONObj<employee_t>(*rootAtom); }

void employees(fixedVector_t<employee_t> &dbEmployees)
{
	jsonArray_t jsonEmployees{};
	for (const auto &dbEmployee : dbEmployees)
		jsonEmployees.add(modelToJSON(dbEmployee).release());
}
