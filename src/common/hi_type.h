#ifndef __HI_TYPE_H__
#define __HI_TYPE_H__

#include <stdint.h>

typedef unsigned char      HI_U8;
typedef unsigned short     HI_U16;
typedef unsigned int       HI_U32;
typedef unsigned long long HI_U64;

typedef signed char        HI_S8;
typedef signed short       HI_S16;
typedef signed int         HI_S32;
typedef signed long long   HI_S64;

typedef float              HI_FLOAT;
typedef double             HI_DOUBLE;

typedef void               HI_VOID;
typedef char               HI_CHAR;

typedef HI_U8              HI_BOOL;
typedef HI_U64             HI_SIZE_T;
typedef HI_S64             HI_SSIZE_T;

#ifdef __cplusplus
#define HI_NULL            nullptr
#else
#define HI_NULL            ((HI_VOID *)0)
#endif
#define HI_TRUE            ((HI_BOOL)1)
#define HI_FALSE           ((HI_BOOL)0)

#define HI_SUCCESS         0
#define HI_FAILURE         (-1)

#define HI_UNUSED(x)       ((HI_VOID)(x))

typedef struct {
    HI_S32 s32X;
    HI_S32 s32Y;
} POINT_S;

typedef struct {
    HI_U32 u32Width;
    HI_U32 u32Height;
} SIZE_S;

typedef struct {
    HI_S32 s32X;
    HI_S32 s32Y;
    HI_U32 u32Width;
    HI_U32 u32Height;
} RECT_S;

#endif /* __HI_TYPE_H__ */
