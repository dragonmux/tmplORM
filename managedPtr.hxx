#ifndef MANAGED_PTR__HXX
#define MANAGED_PTR__HXX

//#include <type_traits>
#include <utility>

namespace managedPtr
{
	using delete_t = void (*)(void *const);

	template<typename T> typename std::enable_if<!std::is_array<T>::value>::type
		deletePtr(void *const object) { delete static_cast<T *>(object); }
	template<typename T> typename std::enable_if<std::is_array<T>::value>::type
		deletePtr(void *const object) { delete [] static_cast<typename std::decay<T>::type>(object); }

	template<typename T> struct managedPtr_t final
	{
	private:
		T *ptr;
		delete_t deleteFunc;
		friend struct managedPtr_t<void>;
		template<typename U> friend struct managedPtr_t;

	public:
		using pointer = T *;
		using reference = T &;

		constexpr managedPtr_t() noexcept : ptr{nullptr}, deleteFunc{nullptr} { }
		managedPtr_t(T *p) noexcept : ptr{p}, deleteFunc{deletePtr<T>} { }
		managedPtr_t(managedPtr_t &&p) noexcept : managedPtr_t{} { swap(p); }
		template<typename U, typename = typename std::enable_if<!std::is_same<T, U>::value &&
				std::is_base_of<T, U>::value>::type>
			managedPtr_t(managedPtr_t<U> &&p) noexcept : managedPtr_t{} { swap(p); }
		~managedPtr_t() noexcept { if (deleteFunc) deleteFunc(ptr); }
		managedPtr_t &operator =(managedPtr_t &&p) noexcept { swap(p); return *this; }

		operator T &() const noexcept { return *ptr; }
		explicit operator T &&() const = delete;
		T &operator *() const noexcept { return *ptr; }
		T *operator ->() const noexcept { return ptr; }
		T *get() noexcept { return ptr; }
		T *get() const noexcept { return ptr; }
		bool valid() const noexcept { return ptr; }
		explicit operator bool() const noexcept { return ptr; }

		void swap(managedPtr_t &p) noexcept
		{
			std::swap(ptr, p.ptr);
			std::swap(deleteFunc, p.deleteFunc);
		}

		template<typename U, typename = typename std::enable_if<!std::is_same<T, U>::value && std::is_base_of<T, U>::value>::type>
			void swap(managedPtr_t<U> &p) noexcept
		{
			if (deleteFunc)
				deleteFunc(ptr);
			ptr = p.ptr;
			deleteFunc = p.deleteFunc;
			p.ptr = nullptr;
			p.deleteFunc = nullptr;
		}

		managedPtr_t(const managedPtr_t &) = delete;
		managedPtr_t &operator =(const managedPtr_t &) = delete;
	};

	template<> struct managedPtr_t<void> final
	{
	private:
		void *ptr;
		delete_t deleteFunc;

	public:
		constexpr managedPtr_t() noexcept : ptr{nullptr}, deleteFunc{nullptr} { }
		managedPtr_t(managedPtr_t &&p) noexcept : managedPtr_t{} { swap(p); }
		template<typename T, typename = typename std::enable_if<!std::is_same<T, void>::value>::type>
			managedPtr_t(managedPtr_t<T> &&p) noexcept : managedPtr_t{} { swap(p); }
		~managedPtr_t() noexcept { if (deleteFunc) deleteFunc(ptr); }
		managedPtr_t &operator =(managedPtr_t &&p) noexcept { swap(p); return *this; }
		template<typename T> managedPtr_t &operator =(T *obj) noexcept { return *this = managedPtr_t<T>(obj); }
		template<typename T, typename = typename std::enable_if<!std::is_same<T, void>::value>::type>
			managedPtr_t &operator =(managedPtr_t<T> &&p) noexcept { swap(p); return *this; }

		operator const void *() const noexcept { return ptr; }
		operator void *() noexcept { return ptr; }
		void *get() noexcept { return ptr; }
		template<typename T> T *get() noexcept { return static_cast<T *>(ptr); }
		void *get() const noexcept { return ptr; }
		template<typename T> T *get() const noexcept { return static_cast<T *const>(ptr); }
		bool valid() const noexcept { return ptr; }
		explicit operator bool() const noexcept { return ptr; }

		void swap(managedPtr_t &p) noexcept
		{
			std::swap(ptr, p.ptr);
			std::swap(deleteFunc, p.deleteFunc);
		}

		template<typename T, typename = typename std::enable_if<!std::is_same<T, void>::value>::type>
			void swap(managedPtr_t<T> &p) noexcept
		{
			if (deleteFunc)
				deleteFunc(ptr);
			ptr = p.ptr;
			deleteFunc = p.deleteFunc;
			p.ptr = nullptr;
			p.deleteFunc = nullptr;
		}

		managedPtr_t(void *) = delete;
		managedPtr_t(const managedPtr_t &) = delete;
		managedPtr_t &operator =(void *) = delete;
		managedPtr_t &operator =(const managedPtr_t &) = delete;
	};
}

template<typename T> using managedPtr_t = managedPtr::managedPtr_t<T>;

template<typename T> struct makeManaged_ { using uniqueType = managedPtr_t<T>; };
template<typename T> struct makeManaged_<T []> { using arrayType = managedPtr_t<T []>; };
template<typename T, size_t N> struct makeManaged_<T [N]> { struct invalidType { }; };

template<typename T, typename... Args> inline typename makeManaged_<T>::uniqueType makeManaged(Args &&...args) noexcept
{
	using consT = typename std::remove_const<T>::type;
	return managedPtr_t<T>(new (std::nothrow) consT(std::forward<Args>(args)...));
}

template<typename T> inline typename makeManaged_<T>::arrayType makeManaged(const size_t num) noexcept
{
	using consT = typename std::remove_const<typename std::remove_extent<T>::type>::type;
	return managedPtr_t<T>(new (std::nothrow) consT[num]());
}

template<typename T, typename... Args> inline typename makeManaged_<T>::invalidType makeManaged(Args &&...) noexcept = delete;

#endif /*MANAGED_PTR__HXX*/
