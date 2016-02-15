//#include "img_cache_log.h"
#include "img_cache.h"
#include "img_io_orig.h"

#include <caches/cache.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <memory>
#include <utility>

const size_t cache_size = 1024;

using Cache = ImgCache;
using key_type = ImgCache::key_type;
using value_type = ImgCache::value_type;

Cache& cache_get(TSK_IMG_INFO* a_img_info) {
  return *static_cast<Cache*>(a_img_info->cache);
}

ssize_t read_fully(char* buf, TSK_IMG_INFO* img, TSK_OFF_T off, size_t len) {
  size_t pos = 0;
  for (ssize_t r; pos < len; pos += r) {
    r = img->read(img, off + pos, buf + pos, len - pos);
    if (r == -1) {
      return -1;
    }
  }
  return len;
}

/*
ssize_t fetch_chunk(TSK_IMG_INFO* img, Cache& cache, char* buf, TSK_OFF_T coff, size_t clen) {
  const size_t chunk_size = cache.chunk_size();
  cache.lock();
  const char* chunk = cache.get(coff);
  if (chunk) {
    // cache hit: copy chunk to buffer
    std::memcpy(buf, chunk, clen);
    cache.unlock();
  }
  else {
    // cache miss: read into buffer, copy chunk to cache
    cache.unlock();
    if (read_fully(buf, img, coff, clen) == -1) {
      return -1;
    }
    cache.lock();
    cache.put(coff, buf);
    cache.unlock();
  }
  return clen;
}
*/

class Timer {
public:
  size_t elapsed() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
      stop_time - start_time
    ).count();
  }

  void start() {
    start_time = std::chrono::high_resolution_clock::now();
  }

  void stop() {
    stop_time = std::chrono::high_resolution_clock::now();
  }

private:
  std::chrono::high_resolution_clock::time_point start_time, stop_time;
};

ssize_t read_chunk(TSK_IMG_INFO* img, TSK_OFF_T coff, size_t clen, char* buf, Timer& timer, Cache& cache) {
  timer.start();
  if (read_fully(buf, img, coff, clen) == -1) {
    return -1;
  }
  timer.stop();
  cache.lock();
  cache.put(coff, buf);
  cache.unlock();
  return clen;
}

/**
 * \ingroup imglib
 * Reads data from an open disk image
 * @param a_img_info Disk image to read from
 * @param a_off Byte offset to start reading from
 * @param a_buf Buffer to read into
 * @param a_len Number of bytes to read into buffer
 * @returns -1 on error or number of bytes read
 */
ssize_t sf_img_read(TSK_IMG_INFO* a_img_info, TSK_OFF_T a_off,
                    char* a_buf, size_t a_len)
{
//  return tsk_img_read_orig(a_img_info, a_off, a_buf, a_len);

//  const auto start_time = std::chrono::high_resolution_clock::now();

  ssize_t read_count = 0;
//  ssize_t cached_count = 0;

  /* cache_lock is used for both the cache in IMG_INFO and
   * the shared variables in the img type specific INFO structs.
   * grab it now so that it is held before any reads.
   */
  tsk_take_lock(&a_img_info->cache_lock);
  std::unique_ptr<tsk_lock_t, void(*)(tsk_lock_t*)> lock_guard(
    &a_img_info->cache_lock, tsk_release_lock
  );

  Cache& cache = cache_get(a_img_info);
  const uint64_t chunk_size = cache.chunk_size();

  char* dst = a_buf;
  const char* chunk;
  std::unique_ptr<char[]> cbuf;

  // offset of src end, taking care not to overrun the end of the image
  const TSK_OFF_T send = std::min((TSK_OFF_T)(a_off + a_len), a_img_info->size);
  // offset of chunk containing src end
  const TSK_OFF_T cend = send & ~(chunk_size - 1);

  // current src offset
  TSK_OFF_T soff = a_off;
  // current chunk offset
  TSK_OFF_T coff = a_off & ~(chunk_size - 1);

  if (coff < soff || cend < send) {
    // we will write at least one partial chunk, set up the chunk buffer
    cbuf.reset(new char[chunk_size]);
  }

  size_t clen, len, delta;

  Timer timer;
  ImgCache::Stats stats;
  ++stats.histogram[(size_t)std::log2(a_len)];

  while (soff < send) {
    clen = std::min((TSK_OFF_T) chunk_size, a_img_info->size - coff);
    delta = soff - coff;
    len = std::min(clen - delta, (size_t)(send - soff)); 

    cache.lock();
    timer.start();
    chunk = cache.get(coff);
    if (chunk) {
      // cache hit: copy chunk to buffer
      std::memcpy(dst, chunk + delta, len);
//      timer.stop();
      cache.unlock();
//      stats.hit_ns += timer.elapsed();
//      ++stats.hits;
//      stats.hit_bytes += len;
    }
    else {
      // cache miss: read into buffer, copy chunk to cache
      cache.unlock();

      if (len < chunk_size) {
        // We're reading less than a complete chunk, so either the start
        // or the end of the read is not aligned to a chunk boundary.
        // Read full chunk into the temporary chunk buffer (because we
        // still want to cache a full chunk), then copy the portion we
        // want into dst.
        if (read_chunk(a_img_info, coff, clen, cbuf.get(), timer, cache) == -1) {
          return -1;
        }
        std::memcpy(dst, cbuf.get() + delta, len);
      }
      else {
        // read a complete chunk
        if (read_chunk(a_img_info, coff, clen, dst, timer, cache) == -1) {
          return -1;
        }
      }

      stats.miss_ns += timer.elapsed();
      ++stats.misses;
      stats.miss_bytes += len;
    }

    soff += len;
    coff += clen;
    dst += len;
  }

  read_count = send - a_off;

  cache.lock();
  cache.stats() += stats;
  cache.unlock();

/*
  const auto end_time = std::chrono::high_resolution_clock::now();
  log(a_off, a_len, read_count, cached_count, start_time, end_time);
*/

  return read_count;
}

void sf_img_cache_init(TSK_IMG_INFO* a_img_info) {
  a_img_info->cache = make_cache(cache_size);
}

void sf_img_cache_free(TSK_IMG_INFO* a_img_info) {
  delete static_cast<Cache*>(a_img_info->cache);
  a_img_info->cache = nullptr;
}
