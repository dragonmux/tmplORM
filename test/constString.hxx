#ifndef CONST_STRING__HXX
#define CONST_STRING__HXX

#include <string>
#include <cassert>
#include <memory.h>

struct constString_t final
{
public:
	using traits_type = std::char_traits<char>;
	using value_type = char;
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	using reference = const char &;
	using const_reference = reference;
	using pointer = const char *;
	using const_pointer = pointer;
	using iterator = pointer;
	using const_iterator = pointer;
	using reverse_iterator = std::reverse_iterator<pointer>;
	using const_reverse_iterator = std::reverse_iterator<pointer>;

	static const size_t npos = static_cast<size_t>(-1);

private:
	std::unique_ptr<char []> string_;
	size_t length_;

	constString_t(nullptr_t) noexcept : string_{}, length_{0} { }
	void checkString() const { if (!string_) throw std::bad_alloc(); }
	static size_t length__(const char *const str) noexcept { return str ? strlen(str) + 1 : 0; }
	static size_t length__(const std::unique_ptr<char []> &str) noexcept { return length__(str.get()); }
	const char *data_() const noexcept { return string_.get(); }
	static std::unique_ptr<char []> stringDup(const std::unique_ptr<char []> &str) noexcept { return stringDup(str.get()); }
	static std::unique_ptr<char []> stringDup(const std::unique_ptr<char []> &str, const size_t n) noexcept { return stringDup(str.get(), n); }

	static std::unique_ptr<char []> stringDup(const char *str = "") noexcept
	{
		if (!str)
			str = "";
		auto ret = makeUnique<char []>(strlen(str) + 1);
		if (!ret)
			return nullptr;
		strcpy(ret.get(), str);
		return ret;
	}

	static std::unique_ptr<char []> stringDup(const char *const str, const size_t n) noexcept
	{
		if (!str && n)
			return nullptr;
		auto ret = makeUnique<char []>(n);
		if (!ret)
			return nullptr;
		memcpy(ret.get(), str, n);
		return ret;
	}

	void assign(std::unique_ptr<char []> &str) { assign(str, length__(string_)); }
	void assign(std::unique_ptr<char []> &str, const size_t len)
	{
		if (!str)
			throw std::bad_alloc();
		std::swap(string_, str);
		length_ = len;
	}

public:
	constString_t() : string_{stringDup()}, length_{0} { checkString(); }
	constString_t(const constString_t &str) : string_{stringDup(str.string_, str.length_)}, length_{str.length_} { checkString(); }
	constString_t(const char *const str, const size_t pos, const size_t n) : string_{stringDup(str + pos, n)}, length_{n} { checkString(); }
	constString_t(const char *const str, const size_t n) : string_{stringDup(str, n)}, length_{n} { checkString(); }
	constString_t(const char *const str) : constString_t{str, length__(str)} { }
	constString_t(constString_t &&str) noexcept : constString_t{nullptr} { swap(str); }
	~constString_t() = default;

	/*constString_t(const constString_t &str, const size_t pos) : string_{[]()
		{
			const char *start = str + str.check_(pos, "constString_t::constString_t");
			return stringDup(start, str.length_ - pos);
		}()}, length_{str.length_ - pos} { checkString(); }
	constString_t(const constString_t &str, const size_t pos, const size_t n) : string_{stringDup(str.string_, str.length_)}, length_{str.length_} { checkString(); }*/

	void operator =(const constString_t &str)
	{
		auto copy = stringDup(str.string_, str.length_);
		assign(copy, str.length_);
	}

	void operator =(const char *const str)
	{
		auto copy = stringDup(str);
		assign(copy, str ? length__(str) : 0);
	}

	void operator =(const char c)
	{
		auto str = makeUnique<char []>(1);
		assign(str, 1);
		str[0] = c;
	}

	void operator =(constString_t &&str) noexcept { swap(str); }
	iterator begin() const noexcept { return data_(); }
	iterator end() const noexcept { return begin() + length_; }
	reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
	reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }
	iterator cbegin() const noexcept { return begin(); }
	iterator cend() const noexcept { return end(); }
	reverse_iterator crbegin() const noexcept { return rbegin(); }
	reverse_iterator crend() const noexcept { return rend(); }
	size_t size() const noexcept { return length_; }
	size_t length() const noexcept { return length_; }
	constexpr size_t max_size() const noexcept { return std::numeric_limits<size_t>::max(); }
	void clear() noexcept { *this = constString_t{}; }
	bool empty() const noexcept { return length_ == 0; }

	const char &operator [](const size_t pos) const noexcept
	{
		assert(pos <= length_);
		return string_[pos];
	}

	const char &at(const size_t n) const
		{ return n < length_ ? string_[n] : (throw std::out_of_range("index out of range in constString_t::at()"), string_[0]); }

	const char &front() const noexcept
	{
		assert(!empty());
		return string_[0];
	}

	const char &back() const noexcept
	{
		assert(!empty());
		return (*this)[length_ - 1];
	}

	void swap(constString_t &str) noexcept
	{
		std::swap(string_, str.string_);
		std::swap(length_, str.length_);
	}

	const char *data() const noexcept { return data_(); }
	operator const char *() const noexcept { return data_(); }
	constString_t substr(const size_t pos = 0, const size_t n = npos) const
		{ return {*this, pos, n}; }
};

#endif /*CONST_STRING__HXX*/
