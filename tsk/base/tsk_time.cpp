#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "tsk_base_i.h"

const std::chrono::time_zone* get_tz() {
  // determine our local timezone
  const char* tzenv = std::getenv("TZ");
  return tzenv ? std::chrono::locate_zone(tzenv) : std::chrono::current_zone();
}

struct tm* tsk_localtime(const time_t* tt) {
#if defined(TSK_WIN32)
  // get a time_point from the time_t
  const auto tp = std::chrono::system_clock::from_time_t(*tt);

  // convert the time_point to the local timezone
  const auto zt = std::chrono::zoned_time{get_tz(), tp};
  const auto lt = zt.get_local_time();

  // localtime uses a static tm; we make it thread-safe
  static thread_local std::tm t;
  std::memset(&t, 0, sizeof(t));

  // break up the local time_point into components
  const auto dp = std::chrono::floor<std::chrono::days>(lt);
  const std::chrono::year_month_day ymd{dp};
  const auto hms = std::chrono::hh_mm_ss(lt-dp);

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
#if defined(TSK_WIN32)
  // POSIX.1 8.1.1 requires that mktime() behave as though tzset() has been
  // called; so we call tzset() to ensure that.
  tzset();

  // create a local time
  const auto date = std::chrono::year{t->tm_year + 1900} / (t->tm_mon + 1) / t->tm_mday;

  const auto lt =
    std::chrono::local_days{date} +
    std::chrono::hours{t->tm_hour} +
    std::chrono::minutes{t->tm_min} +
    std::chrono::seconds{t->tm_sec};

  // attach a time zone to the local time
  const auto zt = std::chrono::zoned_time{get_tz(), lt};
  t->tm_isdst = zt.get_info().save != std::chrono::minutes(0);

  // convert the local time to system time
  const auto st = zt.get_sys_time();
  // convert the system time to a time_t
  return std::chrono::system_clock::to_time_t(st);
#else
  return std::mktime(t);
#endif
}
