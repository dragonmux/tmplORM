#include <crunch++.h>
#include <tmplORM.mysql.hxx>
#include "models.hxx"

using namespace models;
using tmplORM::mysql::createTable_;
using tmplORM::mysql::select_;
using tmplORM::mysql::add_;
using tmplORM::mysql::update_;
using tmplORM::mysql::del_;
using tmplORM::mysql::deleteTable_;

template<typename tableName, typename... fields> const char *add(const model_t<tableName, fields...> &) noexcept
	{ return add_<tableName, fields...>::value; }
template<typename tableName, typename... fields> const char *update(const model_t<tableName, fields...> &) noexcept
	{ return update_<tableName, fields...>::value; }

template<typename tableName, typename... fields> const char *del(const model_t<tableName, fields...> &) noexcept
	{ return del_<tableName, fields...>::value; }

category_t category;
supplier_t supplier;
product_t product;
customer_t customer;
shipper_t shipper;
region_t region;
territory_t territory;
employee_t employee;
order_t order;
demographic_t demographic;
customerDemographic_t customerDemographic;

class testMySQLMapper final : public testsuit
{
public:
	void testInsertGen()
	{
		assertEqual(add(category), "INSERT INTO `Categories` (`CategoryName`, `Description`) VALUES (?, ?);");
		assertEqual(add(supplier), "INSERT INTO `Suppliers` (`CompanyName`, `ContactName`, `ContactTitle`, `Address`, `City`, "
			"`Region`, `PostalCode`, `Country`, `Phone`, `Fax`, `HomePage`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
		assertEqual(add(product), "INSERT INTO `Products` (`ProductName`, `SupplierID`, `CategoryID`, `QuantityPerUnit`, "
			"`UnitsInStock`, `UnitsOnOrder`, `ReorderLevel`, `Discontinued`) VALUES (?, ?, ?, ?, ?, ?, ?, ?);");
		assertEqual(add(customer), "INSERT INTO `Customers` (`CustomerID`, `CompanyName`, `ContactName`, `ContactTitle`, "
			"`Address`, `City`, `Region`, `PostalCode`, `Country`, `Phone`, `Fax`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
		assertEqual(add(shipper), "INSERT INTO `Shippers` (`CompanyName`, `Phone`) VALUES (?, ?);");
		assertEqual(add(region), "INSERT INTO `Regions` (`RegionDescription`) VALUES (?);");
		assertEqual(add(territory), "INSERT INTO `Territories` (`TerritoryID`, `TerritoryDescription`, `RegionID`) VALUES (?, ?, ?);");
		assertEqual(add(employee), "INSERT INTO `Employees` (`LastName`, `FirstName`, `Title`, `TitleOfCourtesy`, "
			"`BirthDate`, `HireDate`, `Address`, `City`, `Region`, `PostalCode`, `Country`, `HomePhone`, `Extension`, "
			"`Notes`, `ReportsTo`, `PhotoPath`) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
		assertEqual(add(order), "INSERT INTO `Orders` (`CustomerID`, `EmployeeID`, `OrderDate`, `RequiredDate`, "
			"`ShippedDate`, `ShipVia`, `ShipName`, `ShipAddress`, `ShipCity`, `ShipRegion`, `ShipPostalCode`, `ShipCountry`) "
			"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
		assertEqual(add(demographic), "INSERT INTO `CustomerDemographics` (`CustomerTypeID`, `CustomerDesc`) VALUES (?, ?);");
		assertEqual(add(customerDemographic), "INSERT INTO `CustomerCustDemographics` (`CustomerID`, `CustomerTypeID`) VALUES (?, ?);");
	}

	void testUpdateGen()
	{
		assertEqual(update(category), "UPDATE `Categories` SET `CategoryName` = ?, `Description` = ? WHERE `CategoryID` = ?;");
		assertEqual(update(supplier), "UPDATE `Suppliers` SET `CompanyName` = ?, `ContactName` = ?, `ContactTitle` = ?, "
			"`Address` = ?, `City` = ?, `Region` = ?, `PostalCode` = ?, `Country` = ?, `Phone` = ?, `Fax` = ?, `HomePage` = ? "
			"WHERE `SupplierID` = ?;");
		assertEqual(update(product), "UPDATE `Products` SET `ProductName` = ?, `SupplierID` = ?, `CategoryID` = ?, "
			"`QuantityPerUnit` = ?, `UnitsInStock` = ?, `UnitsOnOrder` = ?, `ReorderLevel` = ?, `Discontinued` = ? WHERE `ProductID` = ?;");
		assertEqual(update(customer), "UPDATE `Customers` SET `CompanyName` = ?, `ContactName` = ?, `ContactTitle` = ?, "
			"`Address` = ?, `City` = ?, `Region` = ?, `PostalCode` = ?, `Country` = ?, `Phone` = ?, `Fax` = ? WHERE `CustomerID` = ?;");
		assertEqual(update(shipper), "UPDATE `Shippers` SET `CompanyName` = ?, `Phone` = ? WHERE `ShipperID` = ?;");
		assertEqual(update(region), "UPDATE `Regions` SET `RegionDescription` = ? WHERE `RegionID` = ?;");
		assertEqual(update(territory), "UPDATE `Territories` SET `TerritoryDescription` = ?, `RegionID` = ? WHERE `TerritoryID` = ?;");
		assertEqual(update(employee), "UPDATE `Employees` SET `LastName` = ?, `FirstName` = ?, `Title` = ?, `TitleOfCourtesy` = ?, "
			"`BirthDate` = ?, `HireDate` = ?, `Address` = ?, `City` = ?, `Region` = ?, `PostalCode` = ?, `Country` = ?, "
			"`HomePhone` = ?, `Extension` = ?, `Notes` = ?, `ReportsTo` = ?, `PhotoPath` = ? WHERE `EmployeeID` = ?;");
		assertEqual(update(order), "UPDATE `Orders` SET `CustomerID` = ?, `EmployeeID` = ?, `OrderDate` = ?, `RequiredDate` = ?, "
			"`ShippedDate` = ?, `ShipVia` = ?, `ShipName` = ?, `ShipAddress` = ?, `ShipCity` = ?, `ShipRegion` = ?, "
			"`ShipPostalCode` = ?, `ShipCountry` = ? WHERE `OrderID` = ?;");
		assertEqual(update(demographic), "UPDATE `CustomerDemographics` SET `CustomerDesc` = ? WHERE `CustomerTypeID` = ?;");
		assertEqual(update(customerDemographic), "");
	}

	void testDeleteGen()
	{
		assertEqual(del(category), "DELETE FROM `Categories` WHERE `CategoryID` = ?;");
		assertEqual(del(supplier), "DELETE FROM `Suppliers` WHERE `SupplierID` = ?;");
		assertEqual(del(product), "DELETE FROM `Products` WHERE `ProductID` = ?;");
		assertEqual(del(customer), "DELETE FROM `Customers` WHERE `CustomerID` = ?;");
		assertEqual(del(shipper), "DELETE FROM `Shippers` WHERE `ShipperID` = ?;");
		assertEqual(del(region), "DELETE FROM `Regions` WHERE `RegionID` = ?;");
		assertEqual(del(territory), "DELETE FROM `Territories` WHERE `TerritoryID` = ?;");
		assertEqual(del(employee), "DELETE FROM `Employees` WHERE `EmployeeID` = ?;");
		assertEqual(del(order), "DELETE FROM `Orders` WHERE `OrderID` = ?;");
		assertEqual(del(demographic), "DELETE FROM `CustomerDemographics` WHERE `CustomerTypeID` = ?;");
		assertEqual(del(customerDemographic), "DELETE FROM `CustomerCustDemographics` WHERE `CustomerID` = ? AND `CustomerTypeID` = ?;");
	}

	void registerTests() final override
	{
		CXX_TEST(testInsertGen)
		CXX_TEST(testUpdateGen)
		CXX_TEST(testDeleteGen)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testMySQLMapper>();
}
