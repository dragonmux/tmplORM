#include "fromJSON.hxx"
#include "helpers.hxx"

using namespace rSON;

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
		const jsonString_t &quantityPerUnit = product["quantityPerUnit"];
		if (productName.len() > 40 || quantityPerUnit.len() > 20 ||
			!validateID(product["productID"]) ||
			!validateIfInt<validateID>(product["supplierID"]) ||
			!validateIfInt<validateID>(product["categoryID"]))
			return false;
	}
	return true;
}

bool fromJSON_t::validateCustomers(const jsonAtom_t &customersAtom) const noexcept
{
	return true;
}
