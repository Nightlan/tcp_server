/**
* epoll_module
* Copyright (c) 2017 engwei, yang (437798348@qq.com).
*
* @version 1.0
* @author engwei, yang
*/

#ifndef EPOLL_COMMON_H__
#define EPOLL_COMMON_H__

#include <assert.h>
#include <stdlib.h>
#include <cstddef>
#include <string>
#include <vector>
#include <map>

#if defined(__osf__)
#include <inttypes.h>
// Define integer types needed for message write/read APIs for VC
#elif _MSC_VER && _MSC_VER < 1600
#ifndef int8_t
typedef signed char int8_t;
#endif
#ifndef int16_t
typedef short int16_t;
#endif
#ifndef int32_t
typedef int int32_t;
#endif
#ifndef int64_t
typedef long long int64_t;
#endif
#ifndef uint8_t
typedef unsigned char uint8_t;
#endif
#ifndef uint16_t
typedef unsigned short uint16_t;
#endif
#ifndef uint32_t
typedef unsigned int uint32_t;
#endif
#ifndef uint64_t
typedef unsigned long long uint64_t;
#endif
#else
#include <stdint.h>
#endif // _MSC_VER


  /// The EPOLL system error codes definition
#define EPOLL_INVALID     (-1)                // Invalid parameter, or API misuse
#define EPOLL_NOMEM       (-2)                // Memory allocation failed
#define EPOLL_EOVERFLOW   (-3)                // Data exceeds the buffer
#define EPOLL_TIMEOUT     (-4)                // Operation timeout, try again 
  // later
#define EPOLL_TERM        (-5)                // The underlying stream has
  // closed/terminated
#define EPOLL_EOF         (-6)                // End of the file/stream reached 
  // unexpected
#define EPOLL_ID_EXISTS   (-7)                // The element with the same id 
  // has exists
#define EPOLL_NO_EXISTS   (-8)                // The requested element does not 
  // exist
#define EPOLL_DATA_LOST   (-9)                // Data lost
#define EPOLL_EINTR       (-10)               // EINTR
#define EPOLL_BUSY        (-11)               // busy
#define EPOLL_NO_DATA     (-12)               // no data
#define EPOLL_FAIL        (-100)              // Operation failed caused by the 
  // internal logic errors

#undef DISALLOW_CONSTRUCTORS
#define DISALLOW_CONSTRUCTORS(TypeName)    \
  TypeName(const TypeName&);                   \
  void operator=(const TypeName&)

/// The ARRAYSIZE(arr) macro returns the # of elements in an array arr.
/// The expression is a compile-time constant, and therefore can be
/// used in defining new arrays, for example.
///
/// KMQ_ARRAYSIZE catches a few type errors.  If you see a compiler error
///
///   "warning: division by zero in ..."
///
/// when using ARRAYSIZE, you are (wrongfully) giving it a pointer.
/// You should only use EPOLL_ARRAYSIZE on statically allocated arrays.

#define ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
  static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

// Check the return value, we do not log here as the error
// should have been logged when generating the error code
#define CHECK_RESULT(EXPRESSION) \
{int ret=(EXPRESSION);if(0 != ret) return ret;}

/// Gets current time in microseconds from system startup
int64_t GetCurrentMicroseconds();

//set socket nonblocking
int MakeSocketNonBlocking(int sfd);

#endif // !EPOLL_COMMON_H__


