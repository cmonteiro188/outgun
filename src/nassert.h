#ifndef NASSERT_H_INC
#define NASSERT_H_INC

#ifdef NDEBUG
 #define nAssert(expr) ((void)0)
 #define numAssert(expr, val1) ((void)0)
 #define numAssert2(expr, val1, val2) ((void)0)
 #define numAssert3(expr, val1, val2, val3) ((void)0)
 #define numAssert4(expr, val1, val2, val3, val4) ((void)0)
#else // NDEBUG
 #define ARGP const char*, int
 void nAssertFail(const char* expr, const char* file, int line);
 void nAssertFail(const char* expr, ARGP, const char* file, int line);
 void nAssertFail(const char* expr, ARGP, ARGP, const char* file, int line);
 void nAssertFail(const char* expr, ARGP, ARGP, ARGP, const char* file, int line);
 void nAssertFail(const char* expr, ARGP, ARGP, ARGP, ARGP, const char* file, int line);
 #undef ARGP
 #define nAssert(expr) \
    ((expr)?(void)0:nAssertFail(#expr, __FILE__, __LINE__))
 #define numAssert(expr, val1) \
    ((expr)?(void)0:nAssertFail(#expr, #val1, val1, __FILE__, __LINE__))
 #define numAssert2(expr, val1, val2) \
    ((expr)?(void)0:nAssertFail(#expr, #val1, val1, #val2, val2, __FILE__, __LINE__))
 #define numAssert3(expr, val1, val2, val3) \
    ((expr)?(void)0:nAssertFail(#expr, #val1, val1, #val2, val2, #val3, val3, __FILE__, __LINE__))
 #define numAssert4(expr, val1, val2, val3, val4) \
    ((expr)?(void)0:nAssertFail(#expr, #val1, val1, #val2, val2, #val3, val3, #val4, val4, __FILE__, __LINE__))
#endif // NDEBUG

#undef assert
#define assert Use_nAssert_instead_of_assert_please

#endif
