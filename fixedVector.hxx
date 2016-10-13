#ifndef FIXED_VECTOR__HXX
#define FIXED_VECTOR__HXX

#include <memory>
#include <utility>

template<typename T> struct fixedVector_t final
{
private:
	std::unique_ptr<T []> _data;
	size_t _length;

public:
	fixedVector_t() noexcept : _data(), _length(0) { }
	fixedVector_t(size_t length) noexcept : _data(new T[length]()), _length(length) { }
	fixedVector_t(fixedVector_t &&vec) noexcept : fixedVector_t() { swap(vec); }
	~fixedVector_t() noexcept { }
	fixedVector_t &operator =(fixedVector_t &&vec) noexcept { swap(vec); return *this; }

	T &operator [](const size_t index)
	{
		if (index < _length)
			return _data[index];
		throw std::out_of_range();
	}

	size_t length() const noexcept { _length; }
	size_t size() const noexcept { _length; }

	void swap(fixedVector_t &vec) noexcept
	{
		std::swap(_data, vec._data);
		std::swap(_length, vec._length);
	}

	fixedVector_t(const fixedVector_t &) = delete;
	fixedVector_t &operator =(const fixedVector_t &) = delete;
};

#endif /*FIXED_VECTOR__HXX*/
