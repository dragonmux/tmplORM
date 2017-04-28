#ifndef MANAGED_PTR__HXX
#define MANAGED_PTR__HXX

//#include <type_traits>
#include <utility>

namespace managedPtr
{
	using delete_t = void (*)(void *const);

	/*template<typename T>
		typename std::enable_if<!std::is_array<T>::value>::type deletePtr(void *const objectPtr)
	{
		T *const object = reinterpret_cast<T *const>(objectPtr);
		delete object;
	}

	template<typename T>
		typename std::enable_if<std::is_array<T>::value>::type deletePtr(void *const objectPtr)
	{
		T *const object = reinterpret_cast<T *const>(objectPtr);
		delete [] object;
	}*/

	template<typename T> struct managedPtr_t
	{
	private:
		T *ptr;
		delete_t deleteFunc;
		friend struct managedPtr_t<void>;
		template<typename U> friend struct managedPtr_t;

		template<typename U = T> static void del(void *const object)
		{
			if (object)
				static_cast<U *>(object)->~U();
			operator delete(object);
		}

	public:
		using pointer = T *;
		using reference = T &;

		constexpr managedPtr_t() noexcept : ptr(nullptr), deleteFunc(nullptr) { }
		managedPtr_t(T *p) noexcept : ptr(p), deleteFunc(del<T>) { }
		managedPtr_t(managedPtr_t &&p) noexcept : managedPtr_t() { swap(p); }
		template<typename U, typename = typename std::enable_if<!std::is_same<T, U>::value && std::is_base_of<T, U>::value>::type>
			managedPtr_t(managedPtr_t<U> &&p) noexcept : managedPtr_t() { swap(p); }
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

	template<> struct managedPtr_t<void>
	{
	private:
		void *ptr;
		delete_t deleteFunc;

	public:
		constexpr managedPtr_t() noexcept : ptr(nullptr), deleteFunc(nullptr) { }
		managedPtr_t(managedPtr_t &&p) noexcept : managedPtr_t() { swap(p); }
		template<typename T, typename = typename std::enable_if<!std::is_same<T, void>::value>::type>
			managedPtr_t(managedPtr_t<T> &&p) noexcept : managedPtr_t() { swap(p); }
		~managedPtr_t() noexcept { if (deleteFunc) deleteFunc(ptr); }
		managedPtr_t &operator =(managedPtr_t &&p) noexcept { swap(p); return *this; }
		template<typename T> managedPtr_t &operator =(T *obj) noexcept { return *this = managedPtr_t<T>(obj); }
		template<typename T, typename = typename std::enable_if<!std::is_same<T, void>::value>::type>
			managedPtr_t &operator =(managedPtr_t<T> &&p) noexcept { swap(p); return *this; }

		operator const void *() const noexcept { return ptr; }
		operator void *() noexcept { return ptr; }
		void *get() noexcept { return ptr; }
		template<typename T> T *get() noexcept { return reinterpret_cast<T *>(ptr); }
		void *get() const noexcept { return ptr; }
		template<typename T> T *get() const noexcept { return reinterpret_cast<T *const>(ptr); }
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

#endif /*MANAGED_PTR__HXX*/
