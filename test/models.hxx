#include <tmplORM.hxx>

namespace models
{
	using namespace tmplORM;

	struct category_t : public model_t<ts("Categories"),
		types::autoInc_t<types::primary_t<types::int32_t<ts("CategoryID")>>>, types::unicode_t<ts("CategoryName"), 15>,
		types::nullable_t<types::unicodeText_t<ts("Description")>>//, types::nullable_t<types::binary_t<ts("Picture")>>
	> { };

	struct supplier_t : public model_t<ts("Suppliers"),
		types::autoInc_t<types::primary_t<types::int32_t<ts("SupplierID")>>>, types::unicode_t<ts("CompanyName"), 40>,
		types::nullable_t<types::unicode_t<ts("ContactName"), 30>>, types::nullable_t<types::unicode_t<ts("ContactTitle"), 30>>,
		types::nullable_t<types::unicode_t<ts("Address"), 60>>, types::nullable_t<types::unicode_t<ts("City"), 15>>,
		types::nullable_t<types::unicode_t<ts("Region"), 15>>, types::nullable_t<types::unicode_t<ts("PostalCode"), 10>>,
		types::nullable_t<types::unicode_t<ts("Country"), 15>>, types::nullable_t<types::unicode_t<ts("Phone"), 24>>,
		types::nullable_t<types::unicode_t<ts("Fax"), 24>>, types::nullable_t<types::unicodeText_t<ts("HomePage")>>
	> { };

	struct product_t : public model_t<ts("Products"),
		types::autoInc_t<types::primary_t<types::int32_t<ts("ProductID")>>>, types::unicode_t<ts("ProductName"), 40>,
		types::nullable_t<types::int32_t<ts("SupplierID")>>, types::nullable_t<types::int32_t<ts("CategoryID")>>,
		types::nullable_t<types::unicode_t<ts("QuantityPerUnit"), 20>>,// types::nullable_t<types:: <ts("UnitPrice")>>,
		types::nullable_t<types::int16_t<ts("UnitsInStock")>>, types::nullable_t<types::int16_t<ts("UnitsOnOrder")>>,
		types::nullable_t<types::int16_t<ts("ReorderLevel")>>, types::bool_t<ts("Discontinued")>
	> { };

	struct customer_t : public model_t<ts("Customers"),
		types::primary_t<types::unicode_t<ts("CustomerID"), 5>>, types::unicode_t<ts("CompanyName"), 40>,
		types::nullable_t<types::unicode_t<ts("ContactName"), 30>>, types::nullable_t<types::unicode_t<ts("ContactTitle"), 30>>,
		types::nullable_t<types::unicode_t<ts("Address"), 60>>, types::nullable_t<types::unicode_t<ts("City"), 15>>,
		types::nullable_t<types::unicode_t<ts("Region"), 15>>, types::nullable_t<types::unicode_t<ts("PostalCode"), 10>>,
		types::nullable_t<types::unicode_t<ts("Country"), 15>>, types::nullable_t<types::unicode_t<ts("Phone"), 24>>,
		types::nullable_t<types::unicode_t<ts("Fax"), 24>>
	> { };

	struct shipper_t : public model_t<ts("Shippers"),
		types::autoInc_t<types::primary_t<types::int32_t<ts("ShipperID")>>>, types::unicode_t<ts("CompanyName"), 40>,
		types::nullable_t<types::unicode_t<ts("Phone"), 24>>
	> { };

	struct demographic_t : public model_t<ts("CustomerDemographics"),
		types::primary_t<types::unicode_t<ts("CustomerTypeID"), 10>>, types::nullable_t<types::unicodeText_t<ts("CustomerDesc")>>
	> { };

	//struct customerDemographic_t : public model_t<ts("CustomerCustDemographics"),
}
