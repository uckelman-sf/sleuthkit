#include <unistd.h>

#include <fstream>
#include <mutex>
#include <ostream>

#include "img_cache_log.h"

class CacheLogger {
public:
  CacheLogger(std::ostream& out):
    total_req(0), total_hit(0), total_b_req(0),
    total_b_cached(0), total_time(0), out(out)
  {

  }

  ~CacheLogger() {
    out << total_req << '\t'
        << total_hit << '\t'
        << total_b_req << '\t'
        << total_b_cached << '\t'
        << total_time << '\n';
  }

  void log(
    TSK_OFF_T off,   // the requested offset
    size_t b_req,    // bytes requested
    size_t b_ret,    // bytes returned
    size_t b_cached, // bytes read from cache
    const std::chrono::high_resolution_clock::time_point& start,
    const std::chrono::high_resolution_clock::time_point& stop)
  {
    std::lock_guard<std::mutex> lock(m);
    out << off << '\t' << b_req << '\t' << b_ret << '\t'
        << b_cached << '\t' << (stop - start).count() << '\n';

    ++total_req;
    if (b_cached) {
      ++total_hit;
    }
    total_b_req += b_req;
    total_b_cached += b_cached;
    total_time += (stop - start).count();
  }

private:
  size_t total_req, total_hit, total_b_req, total_b_cached;
  uint64_t total_time;
  std::ostream& out;
  std::mutex m;
};

void log(
  TSK_OFF_T off,   // the requested offset
  size_t b_req,    // bytes requested
  size_t b_ret,    // bytes returned
  size_t b_cached, // bytes read from cache
  const std::chrono::high_resolution_clock::time_point& start,
  const std::chrono::high_resolution_clock::time_point& stop)
{
  // local static guarantees file gets opened and closed once, thread-safely 
  static std::ofstream log(
    "cache_log_" + std::to_string(getpid()) + ".txt", std::ios::out
  );

  static CacheLogger logger(log);

  logger.log(off, b_req, b_ret, b_cached, start, stop);
}
