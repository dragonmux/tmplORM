#ifndef VALUE__HXX
#define VALUE__HXX

template<typename T, typename E> struct valueOrError_t
{
private:
	T _value;
	E _error;

public:
	constexpr valueOrError_t() noexcept : _value(), _error() { }
	constexpr valueOrError_t(T value) noexcept : _value(value), _error() { }
	constexpr valueOrError_t(E error) noexcept : _value(), _error(error) { }
	valueOrError_t(valueOrError_t &&v) noexcept : valueOrError_t() { swap(v); }
	valueOrError_t &operator =(valueOrError_t &&v) noexcept { swap(v); return *this; }

	T value() const noexcept { return _value; }
	E error() const noexcept { return _error; }
	bool isError() const noexcept { return _error != E(); }
	operator T() const noexcept { return _value; }

	void swap(valueOrError_t &v) noexcept
	{
		std::swap(_value, v._value);
		std::swap(_error, v._error);
	}

	valueOrError_t(const valueOrError_t &) = delete;
	valueOrError_t &operator =(const valueOrError_t &) = delete;
};

#endif /*VALUE__HXX*/
