#include <chrono>

#include "tsk_img_i.h"

void log(
  TSK_OFF_T off,   // the requested offset
  size_t b_req,    // bytes requested
  size_t b_ret,    // bytes returned
  size_t b_cached, // bytes read from cache
  const std::chrono::high_resolution_clock::time_point& start,
  const std::chrono::high_resolution_clock::time_point& stop
);
