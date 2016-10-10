inline namespace common
{
	template<size_t N> struct selectList__t
	{
		template<typename fieldName, typename T> static auto value(const type_t<fieldName, T> &) ->
			typename fieldName_t<N, type_t<fieldName, T>>::value;
	};
	template<size_t N, typename T> using selectList__ = decltype(selectList__t<N>::value(T()));

	template<size_t N, typename field, typename... fields> struct selectList_t
		{ using value = tycat<selectList__<N, field>, typename selectList_t<N - 1, fields...>::value>; };
	template<typename field> struct selectList_t<1, field> { using value = selectList__<1, field>; };

	template<size_t N> struct insertList__t
	{
		template<typename fieldName, typename T> static auto value(const type_t<fieldName, T> &) ->
			typename fieldName_t<N, type_t<fieldName, T>>::value;
		template<typename T> static auto value(const primary_t<T> &) -> typestring<>;
	};
	template<size_t N, typename T> using insertList__ = decltype(insertList__t<N>::value(T()));

	template<size_t N, typename field, typename... fields> struct insertList_t
		{ using value = tycat<insertList__<N, field>, typename insertList_t<N - 1, fields...>::value>; };
	template<typename field> struct insertList_t<1, field> { using value = insertList__<1, field>; };

	template<typename fieldName, typename T> constexpr size_t countInsert_(const type_t<fieldName, T> &) noexcept { return 1; }
	template<typename T> constexpr size_t countInsert_(const autoInc_t<T> &) noexcept { return 0; }
	template<typename...> struct countInsert_t;
	template<typename field, typename... fields> struct countInsert_t<field, fields...>
		{ constexpr static const size_t count = countInsert_(field()) + countInsert_t<fields...>::count; };
	template<> struct countInsert_t<> { constexpr static const size_t count = 0; };

	template<size_t N> struct updateList__t
	{
		template<typename fieldName, typename T> static auto value(const type_t<fieldName, T> &) ->
			tycat<typename fieldName_t<1, type_t<fieldName, T>>::value, ts(" = "), placeholder<1>, comma<N>>;
		template<typename T> static auto value(const autoInc_t<T> &) -> typestring<>;
	};
	template<size_t N, typename T> using updateList__ = decltype(updateList__t<N>::value(T()));

	template<size_t N, typename field, typename... fields> struct updateList_t
		{ using value = tycat<updateList__<N, field>, typename updateList_t<N - 1, fields...>::value>; };
	template<typename field> struct updateList_t<1, field> { using value = updateList__<1, field>; };

	template<typename fieldName, typename T> constexpr size_t countUpdate_(const type_t<fieldName, T> &) noexcept { return 1; }
	template<typename T> constexpr size_t countUpdate_(const primary_t<T> &) noexcept { return 0; }
	template<typename...> struct countUpdate_t;
	template<typename field, typename... fields> struct countUpdate_t<field, fields...>
		{ constexpr static const size_t count = countUpdate_(field()) + countUpdate_t<fields...>::count; };
	template<> struct countUpdate_t<> { constexpr static const size_t count = 0; };

	template<typename... fields> using selectList = typename selectList_t<sizeof...(fields), fields...>::value;
	template<typename... fields> using insertList = typename insertList_t<sizeof...(fields), fields...>::value;
	template<typename... fields> using updateList = typename updateList_t<sizeof...(fields), fields...>::value;

	template<size_t N, typename field, typename... fields> struct idField_t
		{ using value = typename idField_t<N - 1, fields...>::type; };
	template<typename field, typename... fields> struct idField_t<0, field, fields...>
		{ using value = updateList<toType<field>>; };
	template<typename... fields> using idField = typename idField_t<primaryIndex_t<fields...>::index, fields...>::value;

	template<bool, size_t N, typename field> struct maybeIDField_t
		{ using value = tycat<idField<field>, and_<N>>; };
	template<size_t N, typename field> struct maybeIDField_t<false, N, field> { using value = typestring<>; };
	template<size_t N, typename field> using maybeIDField = typename maybeIDField_t<isPrimaryKey(field()), N, field>::value;

	template<size_t, typename...> struct idFields_t;
	template<size_t N, typename... fields> using idFields_ = typename idFields_t<N, fields...>::value;
	template<size_t N, typename field, typename... fields> struct idFields_t<N, field, fields...>
		{ using value = tycat<maybeIDField<N, field>, idFields_<N - (isPrimaryKey(field()) ? 1 : 0), fields...>>; };
	template<> struct idFields_t<0> { using value = typestring<>; };
	template<typename... fields> using idFields = idFields_<countPrimary<fields...>::count, fields...>;

	template<bool, typename... fields> struct updateWhere_t { };
	template<typename... fields> struct updateWhere_t<true, fields...>
		{ using value = tycat<ts(" WHERE "), idFields<fields...>>; };
	template<typename... fields> using updateWhere = typename updateWhere_t<hasPrimaryKey<fields...>(), fields...>::value;
}
