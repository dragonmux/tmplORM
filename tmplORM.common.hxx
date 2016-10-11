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
	// Alias to make the above easier to use
	template<typename... fields> using selectList = typename selectList_t<sizeof...(fields), fields...>::value;

	template<size_t N> struct insertList__t
	{
		template<typename fieldName, typename T> static auto value(const type_t<fieldName, T> &) ->
			typename fieldName_t<N, type_t<fieldName, T>>::value;
		template<typename T> static auto value(const autoInc_t<T> &) -> typestring<>;
	};
	template<size_t N, typename T> using insertList__ = decltype(insertList__t<N>::value(T()));

	template<size_t N, typename field, typename... fields> struct insertList_t
		{ using value = tycat<insertList__<N, field>, typename insertList_t<N - 1, fields...>::value>; };
	template<typename field> struct insertList_t<1, field> { using value = insertList__<1, field>; };
	// Alias to make the above easier to use
	template<typename... fields> using insertList = typename insertList_t<sizeof...(fields), fields...>::value;

	template<typename fieldName, typename T> constexpr size_t countInsert_(const type_t<fieldName, T> &) noexcept { return 1; }
	template<typename T> constexpr size_t countInsert_(const autoInc_t<T> &) noexcept { return 0; }
	template<typename...> struct countInsert_t;
	template<typename field, typename... fields> struct countInsert_t<field, fields...>
		{ constexpr static size_t count = countInsert_(field()) + countInsert_t<fields...>::count; };
	template<> struct countInsert_t<> { constexpr static size_t count = 0; };

	template<size_t N> struct updateList__t
	{
		template<typename fieldName, typename T> static auto value(const type_t<fieldName, T> &) ->
			tycat<typename fieldName_t<1, type_t<fieldName, T>>::value, ts(" = "), placeholder<1>, comma<N>>;
		template<typename T> static auto value(const primary_t<T> &) -> typestring<>;
	};
	template<size_t N, typename T> using updateList__ = decltype(updateList__t<N>::value(T()));

	template<size_t N, typename field, typename... fields> struct updateList_t
		{ using value = tycat<updateList__<N, field>, typename updateList_t<N - 1, fields...>::value>; };
	template<typename field> struct updateList_t<1, field> { using value = updateList__<1, field>; };
	// Alias to make the above easier to use
	template<typename... fields> using updateList = typename updateList_t<sizeof...(fields), fields...>::value;

	template<typename fieldName, typename T> constexpr size_t countUpdate_(const type_t<fieldName, T> &) noexcept { return 1; }
	template<typename T> constexpr size_t countUpdate_(const primary_t<T> &) noexcept { return 0; }
	template<typename...> struct countUpdate_t;
	template<typename field, typename... fields> struct countUpdate_t<field, fields...>
		{ constexpr static size_t count = countUpdate_(field()) + countUpdate_t<fields...>::count; };
	template<> struct countUpdate_t<> { constexpr static size_t count = 0; };

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

	template<bool, typename tableName, typename... fields> struct update_t { using value = typestring<>; };
	template<typename tableName, typename... fields> using update_ = toString<typename update_t<sizeof...(fields) == countPrimary<fields...>::count, tableName, fields...>::value>;

	template<size_t idx, typename... fields_t> struct bindSelect_t
	{
		constexpr static size_t index = idx - 1;

		template<typename result_t> static void bind(std::tuple<fields_t...> &fields, result_t &result) noexcept
		{
			bindSelect_t<index, fields_t...>::bind(fields, result);
			std::get<index>(fields) = result[index];
		}
	};

	template<typename... fields> struct bindSelect_t<0, fields...>
		{ template<typename result_t> static void bind(std::tuple<fields...> &, result_t &) { } };
	template<typename... fields> using bindSelect = bindSelect_t<sizeof...(fields), fields...>;

	template<size_t bindIndex, typename field_t> struct bindField_t
	{
		template<typename query_t, typename std::enable_if<!field_t::nullable, void *>::type = nullptr>
			static void bind(const field_t &field, query_t &query) noexcept
		{ query.bind(bindIndex, field.value()); }

		template<typename query_t, typename std::enable_if<field_t::nullable, void *>::type = nullptr>
			static void bind(const field_t &field, query_t &query) noexcept
		{
			if (field.isNull())
				query.bind<typename decltype(field)::type>(bindIndex, nullptr);
			else
				query.bind(bindIndex, field.value());
		}
	};

	template<size_t idx, size_t bindIdx, typename... fields_t> struct bindInsert_t
	{
		constexpr static size_t index = idx - 1;
		constexpr static size_t bindIndex = bindIdx - 1;

		template<typename fieldName, typename T, typename field_t, typename query_t>
			static void bindField(const type_t<fieldName, T> &, const field_t &field, const std::tuple<fields_t...> &fields, query_t &query) noexcept
		{
			bindInsert_t<index, bindIndex, fields_t...>::bind(fields, query);
			bindField_t<bindIdx, field_t>::bind(field, query);
		}

		template<typename T, typename field_t, typename query_t>
			static void bindField(const autoInc_t<T> &, const field_t &, const std::tuple<fields_t...> &fields, query_t &query) noexcept
		{ bindInsert_t<index, bindIdx, fields_t...>::bind(fields, query); }

		template<typename query_t> static void bind(const std::tuple<fields_t...> &fields, query_t &query) noexcept
		{
			const auto &field = std::get<index>(fields);
			bindField(field, field, fields, query);
		}
	};

	template<size_t index, typename... fields_t> struct bindInsert_t<index, 0, fields_t...>
		{ template<typename query_t> static void bind(const std::tuple<fields_t...> &, query_t &) { } };
	template<typename... fields> using bindInsert = bindInsert_t<sizeof...(fields), countInsert_t<fields...>::count, fields...>;

	template<size_t index, size_t bindIndex, typename... fields_t> struct bindUpdate_t
	{
		template<typename fieldName, typename T, typename field_t, typename query_t>
			static void bindField(const type_t<fieldName, T> &, const field_t &field, const std::tuple<fields_t...> &fields, query_t &query) noexcept
		{
			bindUpdate_t<index - 1, bindIndex - 1, fields_t...>::bind(fields, query);
			bindField_t<bindIndex - 1, field_t>::bind(field, query);
		}

		template<typename T, typename field_t, typename query_t>
			static void bindField(const primary_t<T> &, const field_t &field, const std::tuple<fields_t...> &fields, query_t &query) noexcept
		{
			bindField_t<bindIndex - 1, field_t>::bind(field, query);
			bindUpdate_t<index - 1, bindIndex - 1, fields_t...>::bind(fields, query);
			// TODO: This actually won't work quite right.. but it is closer than what we had.
		}

		template<typename query_t> static void bind(const std::tuple<fields_t...> &fields, query_t &query) noexcept
		{
			const auto &field = std::get<index>(fields);
			bindField(field, field, fields, query);
		}
	};

	template<size_t index, typename... fields_t> struct bindUpdate_t<index, 0, fields_t...>
		{ template<typename query_t> static void bind(const std::tuple<fields_t...> &, query_t &) { } };
	template<typename... fields> using bindUpdate = bindUpdate_t<sizeof...(fields), countUpdate_t<fields...>::count, fields...>;
}
