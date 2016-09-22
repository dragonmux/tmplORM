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
	template<typename field, typename... fields> struct countInsert_t
		{ constexpr static const size_t count = countInsert_(field()) + countInsert_t<fields...>::count; };
	template<typename field> struct countInsert_t<field> { constexpr static const size_t count = countInsert_(field()); };
}
