

/*******************************************************************

                  Generic Type Definitions


 File Description:

 Change History:
  Rev   Date         Description

*******************************************************************/

#include "cmd.h"

#ifndef __GENERIC_TYPE_DEFS_H_
#define __GENERIC_TYPE_DEFS_H_

/* Specify an extension for GCC based compilers */
#if defined(__GNUC__)
#define __EXTENSION __extension__
#else
#define __EXTENSION
#endif

#if !defined(__PACKED)
    #define __PACKED
#endif

/* get compiler defined type definitions (NULL, size_t, etc) */
#include <stddef.h> 


typedef enum _BOOL { FALSE = 0, TRUE } BOOL;    /* Undefined size */
//typedef enum _BIT { CLEAR = 0, SET } BIT;
typedef enum _BIT { BITCLEAR = 0, BITSET } BIT;//

#define PUBLIC                                  /* Function attributes */
#define PROTECTED
#define PRIVATE   static

/* INT is processor specific in length may vary in size */
typedef signed int          INT;
typedef signed char         INT8;
//typedef unsigned char         uint8;
typedef signed short int    INT16;
//typedef unsigned short int    uint16;
typedef signed long int     INT32;
//typedef unsigned long int     uint32;






#endif /* __GENERIC_TYPE_DEFS_H_ */
