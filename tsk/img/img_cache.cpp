#include "img_cache.h"

//#include "cache.h"
//#include "no_cache.h"

#include <array>
//#include <chrono>
#include <cmath>
#include <cstring>
#include <memory>

#include <list>
#include <unordered_map>
#include <utility>

const size_t CACHE_SIZE = 1024;
const size_t CHUNK_SIZE = 65536;

/*
struct Stats {
  size_t hit_ns     = 0;
  size_t hits       = 0;
  size_t hit_bytes  = 0;
  size_t miss_ns    = 0;
  size_t misses     = 0;
  size_t miss_bytes = 0;

  size_t histogram[64] = {0};

  Stats& operator+=(const Stats& s) {
    hit_ns += s.hit_ns;
    hits += s.hits;
    hit_bytes += s.hit_bytes;
    miss_ns += s.miss_ns;
    misses += s.misses;
    miss_bytes += s.miss_bytes;
    std::transform(std::begin(histogram),
                   std::end(histogram),
                   std::begin(s.histogram),
                   std::begin(histogram),
                   std::plus<size_t>());
    return *this;
  }
};
*/

class Cache {
public:
  virtual ~Cache() = default;

  virtual const char* get(uint64_t key) = 0;

  virtual void put(uint64_t key, const char* val) = 0;

  virtual size_t chunk_size() const = 0;

  virtual size_t cache_size() const = 0;

  virtual void lock() = 0;

  virtual void unlock() = 0;
};

class NoCache: public Cache {
public:
  NoCache(size_t chunk_size): ch_size(chunk_size) {}

  virtual ~NoCache() = default;

  virtual const char* get(uint64_t key) override { return nullptr; }

  virtual void put(uint64_t key, const char* val) override {}

  virtual size_t chunk_size() const override { return ch_size; } 

  virtual size_t cache_size() const override { return 0; }

  virtual void lock() override {};

  virtual void unlock() override {};

private:
  const size_t ch_size;
};

template <class K,
          class V,
          class L = std::list<std::pair<K, V>>,
          class H = std::unordered_map<K, typename L::iterator>>
class LRUCache {
public:
  typedef K key_type;
  typedef V value_type;
  typedef H hash_type;
  typedef L list_type;

  LRUCache(size_t max):
    N(max)
  {}

  const value_type* get(const key_type& key) {
    auto i = hash.find(key);
    if (i != hash.end()) {
      // found existing key, make its item MRU
      items.splice(items.begin(), items, i->second);
      return &(i->second->second);
    }
    else {
      return nullptr;
    }
  }

  void put(const key_type& key, const value_type& val) {
    // try adding new key to hash
    auto r = hash.emplace(key, items.end());
    if (r.second) {
      // new key inserted
      if (items.size() < N) {
        // allocate a new item
        items.emplace_front(key, V());
      }
      else {
        // reuse LRU item
        items.splice(items.begin(), items, std::prev(items.end()));
        if (items.front().first != key) {
          // remove the key from the reused LRU item
          hash.erase(items.front().first);
        }
      }
      (r.first->second = items.begin())->first = key;
    }
    else {
      // found existing key, reset the value and put the item to the front
      items.splice(items.begin(), items, r.first->second);
    }

    items.front().second = val;
  }

  size_t size() const {
    return N;
  }

  void clear() {
    hash.clear();
  }

  typename std::list<std::pair<key_type, value_type>>::const_iterator begin() const {
    return items.cbegin();
  }

  typename std::list<std::pair<key_type, value_type>>::const_iterator end() const {
    return items.cend();
  }

private:
  const size_t N;

  list_type items;
  hash_type hash;
};

class LRUImgCache: public Cache, LRUCache<uint64_t, std::array<char, CHUNK_SIZE>> {
public:
  LRUImgCache(size_t cache_size): LRUCache(cache_size) {}

  virtual const char* get(uint64_t key) {
    return LRUCache::get(key)->data();
  }

  virtual void put(uint64_t key, const char* val) {
    std::array<char, CHUNK_SIZE> v;
    std::copy(val, val + CHUNK_SIZE, std::begin(v));
    LRUCache::put(key, v);
  }

  virtual size_t cache_size() const {
    return size();
  }

  virtual size_t chunk_size() const {
    return CHUNK_SIZE;
  }

/*
  virtual const Stats& stats() const {
    return the_stats;
  }

  virtual Stats& stats() {
    return the_stats;
  }
*/

  virtual void lock() {}

  virtual void unlock() {}
};

Cache* make_cache(size_t chunk_size, size_t cache_size) {
//  return new NoCache(chunk_size);
  return new LRUImgCache(cache_size);
}

Cache& cache_get(TSK_IMG_INFO* a_img_info) {
  return *static_cast<Cache*>(a_img_info->cache_actual);
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
*/

//ssize_t read_chunk(TSK_IMG_INFO* img, TSK_OFF_T coff, size_t clen, char* buf, Timer& timer, Cache& cache) {
ssize_t read_chunk(TSK_IMG_INFO* img, TSK_OFF_T coff, size_t clen, char* buf, Cache& cache) {
//  timer.start();
  if (read_fully(buf, img, coff, clen) == -1) {
    return -1;
  }
//  timer.stop();
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
ssize_t img_cache_read(
  TSK_IMG_INFO* a_img_info,
  TSK_OFF_T a_off,
  char* a_buf,
  size_t a_len
)
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
  const size_t chunk_size = cache.chunk_size();

  char* dst = a_buf;
  const char* chunk;
  std::unique_ptr<char[]> cbuf;

  // offset of src end, taking care not to overrun the end of the image
  const TSK_OFF_T send = a_off + std::min((TSK_OFF_T)a_len, a_img_info->size);
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

//  Timer timer;
//  ImgCache::Stats stats;
//  ++stats.histogram[(size_t)std::log2(a_len)];

  while (soff < send) {
    clen = std::min((TSK_OFF_T) chunk_size, a_img_info->size - coff);
    delta = soff - coff;
    len = std::min(clen - delta, (size_t)(send - soff)); 

    cache.lock();
//    timer.start();
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
//        if (read_chunk(a_img_info, coff, clen, cbuf.get(), timer, cache) == -1) {
        if (read_chunk(a_img_info, coff, clen, cbuf.get(), cache) == -1) {
          return -1;
        }
        std::memcpy(dst, cbuf.get() + delta, len);
      }
      else {
        // read a complete chunk
//        if (read_chunk(a_img_info, coff, clen, dst, timer, cache) == -1) {
        if (read_chunk(a_img_info, coff, clen, dst, cache) == -1) {
          return -1;
        }
      }

//      stats.miss_ns += timer.elapsed();
//      ++stats.misses;
//      stats.miss_bytes += len;
    }

    soff += len;
    coff += clen;
    dst += len;
  }

  read_count = send - a_off;

//  cache.lock();
//  cache.stats() += stats;
//  cache.unlock();

/*
  const auto end_time = std::chrono::high_resolution_clock::now();
  log(a_off, a_len, read_count, cached_count, start_time, end_time);
*/

  return read_count;
}

void img_cache_init(TSK_IMG_INFO* a_img_info) {
  a_img_info->cache_actual = make_cache(CHUNK_SIZE, CACHE_SIZE);
}

void img_cache_free(TSK_IMG_INFO* a_img_info) {
  delete static_cast<Cache*>(a_img_info->cache_actual);
  a_img_info->cache_actual = nullptr;
}
