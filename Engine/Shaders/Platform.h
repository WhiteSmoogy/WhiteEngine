#ifndef Platform_h
#define Platform_h 1

#define SM5 50
#define SM4 40
#define SM66 66

#ifndef SM_VERSION
#define SM_VERSION SM66
#endif

#if SM_VERSION >= SM5
#define SM5_PROFILE 1
#else
#define SM5_PROFILE 0
#endif

#define INFINITE_FLOAT 1.#INF

#endif
