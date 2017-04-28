#ifndef FIXED_VECTOR__HXX
#define FIXED_VECTOR__HXX

#include <memory>
#include <utility>

template<typename T> struct boundedIterator_t final
{
private:
	T *const _data;
	size_t _index;
	size_t _max;

	constexpr boundedIterator_t(T *const data, const size_t index, const size_t max) noexcept :
		_data(data), _index(index), _max(max) { }

public:
	constexpr boundedIterator_t(T *const data, const size_t max) noexcept : _data(data), _index(0), _max(max) { }
	//boundedIterator_t(boundedIterator_t &&iter) noexcept : boundedIterator_t() { swap(iter); }

	T &operator *() const noexcept { return _data[_index]; }
	T *operator ->() const noexcept { return _data + _index; }
	boundedIterator_t &operator ++() noexcept { if (_index < _max) ++_index; return *this; }
	boundedIterator_t &operator ++(int) noexcept { if (_index < _max) return boundedIterator_t(_data, _index++, _max); return *this; }
	boundedIterator_t &operator --() noexcept { if (_index > 0) --_index; return *this; }
	boundedIterator_t &operator --(int) noexcept { if (_index > 0) return boundedIterator_t(_data, _index--, _max); return *this; }
	T &operator [](const size_t n) const noexcept { return *(boundedIterator_t(_data, _index, _max) += n); }
	boundedIterator_t &operator +=(const size_t n) noexcept { if ((_index + n) <= _max) _index += n; else _index = _max; return *this; }
	boundedIterator_t &operator +(const size_t n) const noexcept { return boundedIterator_t(_data, _index, _max) += n; }
	// The supper complicated looking condition here checkes that the subtraction won't underflow and go super-large, in a way where we don't have to care what size_t really means
	boundedIterator_t &operator -=(const size_t n) noexcept { if ((_index - n) & (1 << ((sizeof(size_t) * 8) - 1))) _index = 0; else _index -= n; return *this; }
	boundedIterator_t &operator -(const size_t n) const noexcept { return boundedIterator_t(_data, _index, _max) -= n; }
};

template<typename T> struct fixedVector_t final
{
private:
	std::unique_ptr<T []> _data;
	size_t _length;

public:
	using value_type = T;
	using reference = T &;
	using const_reference = const T &;
	using pointer = T *;
	using const_pointer = const T *const;
	using iterator = boundedIterator_t<T>;
	using const_iterator = boundedIterator_t<const T>;

	fixedVector_t() noexcept : _data(), _length(0) { }
	fixedVector_t(size_t length) noexcept : _data(new T[length]()), _length(length) { }
	fixedVector_t(fixedVector_t &&vec) noexcept : fixedVector_t() { swap(vec); }
	~fixedVector_t() noexcept { }
	fixedVector_t &operator =(fixedVector_t &&vec) noexcept { swap(vec); return *this; }

	size_t length() const noexcept { return _length; }
	size_t size() const noexcept { return _length; }
	bool valid() const noexcept { return bool(_data); }
	T *data() const noexcept { return _data.get(); }

	reference operator [](const size_t index)
	{
		if (index < _length)
			return _data[index];
		throw std::out_of_range("index out of range");
	}

	const_reference operator [](const size_t index) const
	{
		if (index < _length)
			return _data[index];
		throw std::out_of_range("index out of range");
	}

	iterator begin() noexcept { return iterator(_data.get(), _length); }
	const_iterator begin() const noexcept { return const_iterator(_data.get(), _length); }
	iterator end() noexcept { return begin() + _length; }
	const_iterator end() const noexcept { return begin() + _length; }

	void swap(fixedVector_t &vec) noexcept
	{
		std::swap(_data, vec._data);
		std::swap(_length, vec._length);
	}

	fixedVector_t(const fixedVector_t &) = delete;
	fixedVector_t &operator =(const fixedVector_t &) = delete;
};

#endif /*FIXED_VECTOR__HXX*/
