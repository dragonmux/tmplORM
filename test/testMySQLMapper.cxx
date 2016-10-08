#include <crunch++.h>
#include <tmplORM.mysql.hxx>
#include "models.hxx"

using namespace models;
using tmplORM::mysql::createTable__;
using tmplORM::mysql::select__;
using tmplORM::mysql::add__;
using tmplORM::mysql::update__;
using tmplORM::mysql::del__;
using tmplORM::mysql::deleteTable__;

template<typename tableName, typename... fields> const char *add(const model_t<tableName, fields...> &) noexcept
	{ return add__<tableName, fields...>::value; }
template<typename tableName, typename... fields> const char *update(const model_t<tableName, fields...> &) noexcept
	{ return update__<tableName, fields...>::value; }

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

class testMySQLMapper final : public testsuit
{
public:
	void testInsertGen()
	{
		puts(add(category));
		puts(add(supplier));
		puts(add(product));
		puts(add(customer));
		puts(add(shipper));
		puts(add(region));
		puts(add(territory));
		puts(add(employee));
		puts(add(order));
		puts(add(demographic));
	}

	void testUpdateGen()
	{
		puts(update(category));
		puts(update(supplier));
		puts(update(product));
		puts(update(customer));
		puts(update(shipper));
		puts(update(region));
		puts(update(territory));
		puts(update(employee));
		puts(update(order));
		puts(update(demographic));
	}

	void registerTests() final override
	{
		CXX_TEST(testInsertGen)
		CXX_TEST(testUpdateGen)
	}
};

CRUNCH_API void registerCXXTests() noexcept;
void registerCXXTests() noexcept
{
	registerTestClasses<testMySQLMapper>();
}
