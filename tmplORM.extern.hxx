#ifndef tmplORM_EXTERN__HXX
#define tmplORM_EXTERN__HXX

#ifdef _MSC_VER
	// TODO: Define me.
#else
	#if __GNUC__ > 4 || __clang_major__ > 3
		#define tmplORM_DEFAULT_VISIBILITY __attribute__((visibility("default")))
	#else
		#error Unsupported version of GCC - Must use GCC-5 or higher to correctly compile tmplORM.
		#define tmplORM_DEFAULT_VISIBILITY
	#endif
	#define tmplORM_API tmplORM_DEFAULT_VISIBILITY
	#define tmplORM_FNAPI extern tmplORM_API
#endif

#endif /*tmplORM_EXTERN__HXX*/
