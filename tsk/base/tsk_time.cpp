#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <date/date.h>
#include <date/tz.h>

#include "tsk_base_i.h"

struct tm* tsk_localtime(const time_t* tt) {
#if defined(TSK_WIN32) && defined(HAVE_LIBTZ)
  // determine our local timezone
  const char* tzenv = std::getenv("TZ");
  const auto tz = tzenv ? date::locate_zone(tzenv) : date::current_zone();

  // get a time_point from the time_t
  const auto tp = std::chrono::system_clock::from_time_t(*tt);

  // convert the time_point to the local timezone
  const auto zt = date::make_zoned(tz, tp);
  const auto lt = zt.get_local_time();

  // localtime uses a static tm; we make it thread-safe
  static thread_local std::tm t;
  std::memset(&t, 0, sizeof(t));

  // break up the local time_point into components
  const auto dp = date::floor<date::days>(lt);
  const date::year_month_day ymd{dp};
  const auto hms = date::make_time(lt-dp);

  // fill the tm with the components
  t.tm_year = static_cast<int>(ymd.year()) - 1900;
  t.tm_mon = static_cast<unsigned>(ymd.month()) - 1;
  t.tm_mday = static_cast<unsigned>(ymd.day());
  t.tm_hour = hms.hours().count();
  t.tm_min = hms.minutes().count();
  t.tm_sec = hms.seconds().count();
  t.tm_isdst = zt.get_info().save != std::chrono::minutes(0);

  return &t;
#else
  return std::localtime(tt);
#endif
}

time_t tsk_mktime(struct tm* t) {
#if defined(TSK_WIN32) && defined(HAVE_LIBTZ)
  // POSIX.1 8.1.1 requires that mktime() behave as though tzset() has been
  // called; so we call tzset() to ensure that.
  tzset();

  // determine our local timezone
  const char* tzenv = std::getenv("TZ");
  const auto tz = tzenv ? date::locate_zone(tzenv) : date::current_zone();

  // create a local time
  const auto lt =
    date::local_days{date::year{t->tm_year+1900}/(t->tm_mon+1)/t->tm_mday} +
    std::chrono::hours{t->tm_hour} +
    std::chrono::minutes{t->tm_min} +
    std::chrono::seconds{t->tm_sec};

  // attach a time zone to the local time
  const auto zt = date::make_zoned(tz, lt);
  t->tm_isdst = zt.get_info().save != std::chrono::minutes(0);

  // convert the local time to system time
  const auto st = zt.get_sys_time();
  // convert the system time to a time_t
  return std::chrono::system_clock::to_time_t(st);
#else
  return std::mktime(t);
#endif
}
