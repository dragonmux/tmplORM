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

template<typename tableName, typename... fields> const char *createTable(const model_t<tableName, fields...> &) noexcept
	{ return createTable_<tableName, fields...>::value; }
template<typename tableName, typename... fields> const char *add(const model_t<tableName, fields...> &) noexcept
	{ return add_<tableName, fields...>::value; }
template<typename tableName, typename... fields> const char *update(const model_t<tableName, fields...> &) noexcept
	{ return update_<tableName, fields...>::value; }

template<typename tableName, typename... fields> const char *del(const model_t<tableName, fields...> &) noexcept
	{ return del_<tableName, fields...>::value; }
template<typename tableName, typename... fields> const char *deleteTable(const model_t<tableName, fields...> &) noexcept
	{ return deleteTable_<tableName>::value; }

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

class testMySQLMapper final : public testsuite
{
public:
	void testCreateTableGen()
	{
		assertEqual(createTable(category), "CREATE TABLE IF NOT EXISTS `Categories` (`CategoryID` INT PRIMARY KEY AUTO_INCREMENT NOT NULL, "
			"`CategoryName` VARCHAR(15) NOT NULL, `Description`  NULL) CHARACTER SET utf8;");
		assertEqual(createTable(supplier), "CREATE TABLE IF NOT EXISTS `Suppliers` (`SupplierID` INT PRIMARY KEY AUTO_INCREMENT NOT NULL, "
			"`CompanyName` VARCHAR(40) NOT NULL, `ContactName` VARCHAR(30) NULL, `ContactTitle` VARCHAR(30) NULL, `Address` VARCHAR(60) NULL, "
			"`City` VARCHAR(15) NULL, `Region` VARCHAR(15) NULL, `PostalCode` VARCHAR(10) NULL, `Country` VARCHAR(15) NULL, "
			"`Phone` VARCHAR(24) NULL, `Fax` VARCHAR(24) NULL, `HomePage`  NULL) CHARACTER SET utf8;");
		assertEqual(createTable(product), "CREATE TABLE IF NOT EXISTS `Products` (`ProductID` INT PRIMARY KEY AUTO_INCREMENT NOT NULL, "
			"`ProductName` VARCHAR(40) NOT NULL, `SupplierID` INT NULL, `CategoryID` INT NULL, `QuantityPerUnit` VARCHAR(20) NULL, "
			"`UnitsInStock` SMALLINT NULL, `UnitsOnOrder` SMALLINT NULL, `ReorderLevel` SMALLINT NULL, `Discontinued` BIT(1) NOT NULL) CHARACTER SET utf8;");
		assertEqual(createTable(customer), "CREATE TABLE IF NOT EXISTS `Customers` (`CustomerID` VARCHAR(5) PRIMARY KEY NOT NULL, "
			"`CompanyName` VARCHAR(40) NOT NULL, `ContactName` VARCHAR(30) NULL, `ContactTitle` VARCHAR(30) NULL, `Address` VARCHAR(60) NULL, "
			"`City` VARCHAR(15) NULL, `Region` VARCHAR(15) NULL, `PostalCode` VARCHAR(10) NULL, `Country` VARCHAR(15) NULL, "
			"`Phone` VARCHAR(24) NULL, `Fax` VARCHAR(24) NULL) CHARACTER SET utf8;");
		assertEqual(createTable(shipper), "CREATE TABLE IF NOT EXISTS `Shippers` (`ShipperID` INT PRIMARY KEY AUTO_INCREMENT NOT NULL, "
			"`CompanyName` VARCHAR(40) NOT NULL, `Phone` VARCHAR(24) NULL) CHARACTER SET utf8;");
		assertEqual(createTable(region), "CREATE TABLE IF NOT EXISTS `Regions` (`RegionID` INT PRIMARY KEY AUTO_INCREMENT NOT NULL, "
			"`RegionDescription` VARCHAR(50) NOT NULL) CHARACTER SET utf8;");
		assertEqual(createTable(territory), "CREATE TABLE IF NOT EXISTS `Territories` (`TerritoryID` VARCHAR(20) PRIMARY KEY NOT NULL, "
			"`TerritoryDescription` VARCHAR(50) NOT NULL, `RegionID` INT NOT NULL) CHARACTER SET utf8;");
		assertEqual(createTable(employee), "CREATE TABLE IF NOT EXISTS `Employees` (`EmployeeID` INT PRIMARY KEY AUTO_INCREMENT NOT NULL, "
			"`LastName` VARCHAR(20) NOT NULL, `FirstName` VARCHAR(10) NOT NULL, `Title` VARCHAR(30) NULL, `TitleOfCourtesy` VARCHAR(25) NULL, "
			"`BirthDate`  NULL, `HireDate`  NULL, `Address` VARCHAR(60) NULL, `City` VARCHAR(15) NULL, `Region` VARCHAR(15) NULL, "
			"`PostalCode` VARCHAR(10) NULL, `Country` VARCHAR(15) NULL, `HomePhone` VARCHAR(24) NULL, `Extension` VARCHAR(4) NULL, "
			"`Notes`  NULL, `ReportsTo` INT NULL, `PhotoPath` VARCHAR(255) NULL) CHARACTER SET utf8;");
		assertEqual(createTable(order), "CREATE TABLE IF NOT EXISTS `Orders` (`OrderID` INT PRIMARY KEY AUTO_INCREMENT NOT NULL, "
			"`CustomerID` VARCHAR(5) NULL, `EmployeeID` INT NULL, `OrderDate`  NULL, `RequiredDate`  NULL, `ShippedDate`  NULL, "
			"`ShipVia` INT NULL, `ShipName` VARCHAR(40) NULL, `ShipAddress` VARCHAR(60) NULL, `ShipCity` VARCHAR(15) NULL, "
			"`ShipRegion` VARCHAR(15) NULL, `ShipPostalCode` VARCHAR(10) NULL, `ShipCountry` VARCHAR(15) NULL) CHARACTER SET utf8;");
		assertEqual(createTable(demographic), "CREATE TABLE IF NOT EXISTS `CustomerDemographics` (`CustomerTypeID` VARCHAR(10) PRIMARY KEY NOT NULL, "
			"`CustomerDesc`  NULL) CHARACTER SET utf8;");
		assertEqual(createTable(customerDemographic), "CREATE TABLE IF NOT EXISTS `CustomerCustDemographics` (`CustomerID` VARCHAR(5) "
			"PRIMARY KEY NOT NULL, `CustomerTypeID` VARCHAR(10) PRIMARY KEY NOT NULL) CHARACTER SET utf8;");
	}

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

	void testDropTableGen()
	{
		assertEqual(deleteTable(category), "DROP TABLE IF EXISTS `Categories`;");
		assertEqual(deleteTable(supplier), "DROP TABLE IF EXISTS `Suppliers`;");
		assertEqual(deleteTable(product), "DROP TABLE IF EXISTS `Products`;");
		assertEqual(deleteTable(customer), "DROP TABLE IF EXISTS `Customers`;");
		assertEqual(deleteTable(shipper), "DROP TABLE IF EXISTS `Shippers`;");
		assertEqual(deleteTable(region), "DROP TABLE IF EXISTS `Regions`;");
		assertEqual(deleteTable(territory), "DROP TABLE IF EXISTS `Territories`;");
		assertEqual(deleteTable(employee), "DROP TABLE IF EXISTS `Employees`;");
		assertEqual(deleteTable(order), "DROP TABLE IF EXISTS `Orders`;");
		assertEqual(deleteTable(demographic), "DROP TABLE IF EXISTS `CustomerDemographics`;");
		assertEqual(deleteTable(customerDemographic), "DROP TABLE IF EXISTS `CustomerCustDemographics`;");
	}

	void registerTests() final
	{
		CXX_TEST(testCreateTableGen)
		CXX_TEST(testInsertGen)
		CXX_TEST(testUpdateGen)
		CXX_TEST(testDeleteGen)
		CXX_TEST(testDropTableGen)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testMySQLMapper>();
}
