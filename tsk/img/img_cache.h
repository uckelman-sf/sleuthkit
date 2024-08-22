#pragma once

#include "tsk_img_i.h"

#ifdef __cplusplus
extern "C" {
#endif

ssize_t img_cache_read(
  TSK_IMG_INFO* a_img_info,
  TSK_OFF_T a_off,
  char *a_buf,
  size_t a_len
);

void img_cache_init(TSK_IMG_INFO* a_img_info);

void img_cache_free(TSK_IMG_INFO* a_img_info);

#ifdef __cplusplus
}
#endif
