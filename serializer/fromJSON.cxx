#include "fromJSON.hxx"
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
		if (!isIn(key, "products", "customers"))
			return false;
	}

	// Validate primary (required) keys
	// if (!data.exists("key1") || !validateKey1(data["key1"]) ||
	//	!data.exists("key2") || !validateKey2(data["key2"]))

	// Validate secondary (optional) keys
   	if ((data.exists("products") && !validateProducts(data["products"])) ||
   		(data.exists("customers") && !validateCustomers(data["customers"])))
   		return false;

   	return true;
}

bool fromJSON_t::validateProducts(const jsonAtom_t &productsAtom) const noexcept
{
	if (!typeIs<JSON_TYPE_ARRAY>(productsAtom))
		return false;
	const jsonArray_t &products = productsAtom;
	for (const auto productAtom : products)
	{
		if (!typeIs<JSON_TYPE_OBJECT>(*productAtom))
			return false;
		const jsonObject_t &product = *productAtom;

		if (!product.exists("productID") || !product.exists("productName") ||
			!product.exists("supplierID") || !product.exists("categoryID") ||
			!product.exists("quantityPerUnit") || //!product.exists("unitPrice") ||
			!product.exists("unitsInStock") || !product.exists("unitsOnOrder") ||
			!product.exists("reorderLevel") || !product.exists("discontinueed") ||
			!typeIs<JSON_TYPE_INT>(product["productID"]) ||
			!typeIs<JSON_TYPE_STRING>(product["productName"]) ||
			!typeIsOrNull<JSON_TYPE_INT>(product["supplierID"]) ||
			!typeIsOrNull<JSON_TYPE_INT>(product["categoryID"]) ||
			!typeIsOrNull<JSON_TYPE_STRING>(product["quantityPerUnit"]) ||
			//!typeIsOrNull<>(product["unitPrice"]) ||
			!typeIsOrNull<JSON_TYPE_INT>(product["unitsInStock"]) ||
			!typeIsOrNull<JSON_TYPE_INT>(product["unitsOnOrder"]) ||
			!typeIsOrNull<JSON_TYPE_INT>(product["reorderLevel"]) ||
			!typeIs<JSON_TYPE_BOOL>(product["discontinued"]))
			return false;

		const jsonString_t &productName = product["productName"];
		if (productName.len() > 40 ||
			!validateID(product["productID"]) ||
			!validateIfInt<validateID>(product["supplierID"]) ||
			!validateIfInt<validateID>(product["categoryID"]) ||
			(typeIs<JSON_TYPE_STRING>(product["quantityPerUnit"]) &&
			product["quantityPerUnit"].asStringRef().len() > 20))
			return false;
	}
	return true;
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

		if (!customer.exists("customerID") || !customer.exists("companyName") ||
			!customer.exists("contactName") || !customer.exists("contactTitle") ||
			!customer.exists("address") || !customer.exists("city") ||
			!customer.exists("region") || !customer.exists("postalCode") ||
			!customer.exists("country") || !customer.exists("phone") ||
			!customer.exists("fax") ||
			!typeIs<JSON_TYPE_STRING>(customer["customerID"]) ||
			!typeIs<JSON_TYPE_STRING>(customer["companyName"]) ||
			!typeIsOrNull<JSON_TYPE_STRING>(customer["contactName"]) ||
			!typeIsOrNull<JSON_TYPE_STRING>(customer["contactTitle"]) ||
			!typeIsOrNull<JSON_TYPE_STRING>(customer["address"]) ||
			!typeIsOrNull<JSON_TYPE_STRING>(customer["city"]) ||
			!typeIsOrNull<JSON_TYPE_STRING>(customer["region"]) ||
			!typeIsOrNull<JSON_TYPE_STRING>(customer["postalCode"]) ||
			!typeIsOrNull<JSON_TYPE_STRING>(customer["country"]) ||
			!typeIsOrNull<JSON_TYPE_STRING>(customer["phone"]) ||
			!typeIsOrNull<JSON_TYPE_STRING>(customer["fax"]))
			return false;

		const jsonString_t &customerID = customer["customerID"];
		const jsonString_t &companyName = customer["companyName"];
		if (customerID.len() > 5 || companyName.len() > 40 ||
			(typeIs<JSON_TYPE_STRING>(customer["contactName"]) &&
			customer["contactName"].asStringRef().len() > 30) ||
			(typeIs<JSON_TYPE_STRING>(customer["contactTitle"]) &&
			customer["contactTitle"].asStringRef().len() > 30) ||
			(typeIs<JSON_TYPE_STRING>(customer["address"]) &&
			customer["address"].asStringRef().len() > 60) ||
			(typeIs<JSON_TYPE_STRING>(customer["city"]) &&
			customer["city"].asStringRef().len() > 15) ||
			(typeIs<JSON_TYPE_STRING>(customer["region"]) &&
			customer["region"].asStringRef().len() > 15) ||
			(typeIs<JSON_TYPE_STRING>(customer["postalCode"]) &&
			customer["postalCode"].asStringRef().len() > 10) ||
			(typeIs<JSON_TYPE_STRING>(customer["country"]) &&
			customer["country"].asStringRef().len() > 15) ||
			(typeIs<JSON_TYPE_STRING>(customer["phone"]) &&
			customer["phone"].asStringRef().len() > 24) ||
			(typeIs<JSON_TYPE_STRING>(customer["fax"]) &&
			customer["fax"].asStringRef().len() > 24))
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
