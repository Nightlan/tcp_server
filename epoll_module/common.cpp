#include <common.h>
#include <fcntl.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN  // We only need minimal includes
#include <windows.h>
// MSVC has only _snprintf, not snprintf.
//
// MinGW has both snprintf and _snprintf, but they appear to be different
// functions.  The former is buggy.  When invoked like so:
//   char buffer[32];
//   snprintf(buffer, 32, "%.*g\n", FLT_DIG, 1.23e10f);
// it prints "1.23000e+10".  This is plainly wrong:  %g should never print
// trailing zeros after the decimal point.  For some reason this bug only
// occurs with some input values, not all.  In any case, _snprintf does the
// right thing, so we use it.
#define snprintf _snprintf    // see comment in strutil.cc
#else
#include <sys/time.h>
#endif

#if defined HAVE_CLOCK_GETTIME || defined HAVE_GETHRTIME
#include <time.h>
#endif


int64_t GetCurrentMicroseconds() {
#if defined _WIN32
  //  Get the high resolution counter's accuracy.
  LARGE_INTEGER ticksPerSecond;
  QueryPerformanceFrequency(&ticksPerSecond);

  //  What time is it?
  LARGE_INTEGER tick;
  QueryPerformanceCounter(&tick);

  //  Convert the tick number into the number of seconds
  //  since the system was started.
  double ticks_div = (double)(ticksPerSecond.QuadPart / 1000000.0);
  return (int64_t)(tick.QuadPart / ticks_div);

#elif defined HAVE_CLOCK_GETTIME && defined CLOCK_MONOTONIC

  //  Use POSIX clock_gettime function to get precise monotonic time.
  struct timespec tv;
  int rc = clock_gettime(CLOCK_MONOTONIC, &tv);
  // Fix case where system has clock_gettime but CLOCK_MONOTONIC is not supported.
  // This should be a configuration check, but I looked into it and writing an 
  // AC_FUNC_CLOCK_MONOTONIC seems beyond my powers.
  if (rc != 0) {
    //  Use POSIX gettimeofday function to get precise time.
    struct timeval tv;
    int rc = gettimeofday(&tv, nullptr);
    assert(rc == 0);
    return (tv.tv_sec * (int64_t)1000000 + tv.tv_usec);
  }
  return (tv.tv_sec * (int64_t)1000000 + tv.tv_nsec / 1000);

#elif defined HAVE_GETHRTIME

  return (gethrtime() / 1000);

#else

  //  Use POSIX gettimeofday function to get precise time.
  struct timeval tv;
  int rc = gettimeofday(&tv, nullptr);
  assert(rc == 0);
  return (tv.tv_sec * (int64_t)1000000 + tv.tv_usec);

#endif

}


int MakeSocketNonBlocking(int sfd) {
  int flags, rst;

  flags = fcntl(sfd, F_GETFL, 0);
  if (flags == -1) {
    return -1;
  }
  flags |= O_NONBLOCK;
  rst = fcntl(sfd, F_SETFL, flags);
  if (rst == -1) {
    return -1;
  }
  return 0;
}