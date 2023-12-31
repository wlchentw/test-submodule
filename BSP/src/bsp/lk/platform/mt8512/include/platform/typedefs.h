/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2001
*
*****************************************************************************/

/*****************************************************************************
 *
 * Filename:
 * ---------
 *   Error_code.h
 *
 * Project:
 * --------
 *   Device Test
 *
 * Description:
 * ------------
 *   Type definition.
 *
 * Author:
 * -------
 *   Shalyn Chua  (mtk00576)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 *
 ****************************************************************************/


#ifndef _MTK_DVC_TEST_TYPEDEFS_H
#define _MTK_DVC_TEST_TYPEDEFS_H


/*==== CONSTANTS ==================================================*/

#define IMPORT  EXTERN
#ifndef __cplusplus
  #define EXTERN  extern
#else
  #define EXTERN  extern "C"
#endif
#define LOCAL     static
#define GLOBAL
#define EXPORT    GLOBAL


#define EQ        ==
#define NEQ       !=
#define AND       &&
#define OR        ||
#define XOR(A,B)  ((!(A) AND (B)) OR ((A) AND !(B)))

#ifndef FALSE
  #define FALSE   0
#endif

#ifndef TRUE
  #define TRUE    1
#endif

#ifndef NULL
  #define NULL    0
#endif

enum {RX, TX, NONE};

/*==== TYPES ======================================================*/

typedef unsigned char   kal_uint8;
typedef unsigned short  kal_uint16;
typedef unsigned int    kal_uint32;

typedef unsigned long long  kal_uint64;

typedef unsigned char   US8;
typedef unsigned short  US16;
typedef unsigned int    US32;

typedef volatile unsigned char  *P_kal_uint8;
typedef volatile signed char    *P_S8;
typedef volatile unsigned short *P_kal_uint16;
typedef volatile signed short   *P_S16;
typedef volatile unsigned int   *P_kal_uint32;
typedef volatile signed int     *P_S32;

typedef long            LONG;
typedef unsigned char   UBYTE;
typedef short           SHORT;

typedef signed char     kal_int8;
typedef signed short    kal_int16;
typedef signed int      kal_int32;
typedef long long       kal_int64;
typedef char            kal_char;

typedef unsigned int            *UINT32P;
typedef volatile unsigned short *UINT16P;
typedef volatile unsigned char  *UINT8P;
typedef unsigned char           *U8P;

typedef signed long long        *P_S64;
typedef unsigned char   BOOL;
typedef signed char     INT8;
typedef signed short    INT16;
typedef signed int      INT32;
typedef unsigned char       U8;
typedef unsigned short      U16;
typedef unsigned int        U32;
typedef unsigned long long  U64;
typedef signed char     S8;
typedef signed short    S16;
typedef signed int      S32;
typedef signed long long    S64;
typedef enum {
   KAL_FALSE = 0,
   KAL_TRUE  = 1,
   kal_false = KAL_FALSE,
   kal_true  = KAL_TRUE,
} kal_bool;

typedef unsigned int    UINT32;
typedef unsigned short  USHORT;
typedef signed int      DWORD;
typedef void            VOID;
typedef unsigned char   BYTE;
typedef float           FLOAT;

typedef char           *LPCSTR;
typedef short          *LPWSTR;

#endif

