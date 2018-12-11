#include "fromJSON.hxx"
#include "toJSON.hxx"
#include "helpers.hxx"

using namespace rSON;
using tmplORM::modelFromJSON;

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
{
	const jsonObject_t &data = *rootAtom;
	const jsonArray_t &jsonProducts = data["products"];
	fixedVector_t<product_t> dbProducts(jsonProducts.count());
	if (!dbProducts.valid())
		return {};
	for (size_t i = 0; i < jsonProducts.count(); ++i)
		dbProducts[i] = modelFromJSON<product_t>(jsonProducts[i]);
	return dbProducts;
}

bool fromJSON_t::validateCustomers(const jsonAtom_t &customersAtom) const noexcept
{
	if (!typeIs<JSON_TYPE_ARRAY>(customersAtom))
		return false;
	const jsonArray_t &customers = customersAtom;
	for (const auto &customerAtom : customers)
	{
		if (!typeIs<JSON_TYPE_OBJECT>(*customerAtom))
			return false;
		const jsonObject_t &customer = *customerAtom;
		if (!isValidJSON<customer_t>(customer))
			return false;
	}
	return true;
}

fixedVector_t<customer_t> fromJSON_t::customers() const noexcept
{
	const jsonObject_t &data = *rootAtom;
	const jsonArray_t &jsonCustomers = data["customers"];
	fixedVector_t<customer_t> dbCustomers(jsonCustomers.count());
	if (!dbCustomers.valid())
		return {};
	for (size_t i = 0; i < jsonCustomers.count(); ++i)
		dbCustomers[i] = modelFromJSON<customer_t>(jsonCustomers[i]);
	return dbCustomers;
}

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
{
	const jsonObject_t &data = *rootAtom;
	const jsonArray_t &jsonEmployees = data["employees"];
	fixedVector_t<employee_t> dbEmployees(jsonEmployees.count());
	if (!dbEmployees.valid())
		return {};
	for (size_t i = 0; i < jsonEmployees.count(); ++i)
		dbEmployees[i] = modelFromJSON<employee_t>(jsonEmployees[i]);
	return dbEmployees;
}

void employees(fixedVector_t<employee_t> &dbEmployees)
{
	jsonArray_t jsonEmployees{};
	for (const auto &dbEmployee : dbEmployees)
		jsonEmployees.add(modelToJSON(dbEmployee).release());
}
