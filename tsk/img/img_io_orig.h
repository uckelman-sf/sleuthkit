#pragma once

#include "tsk_img_i.h"

#ifdef __cplusplus
extern "C" {
#endif

ssize_t
tsk_img_read_orig(
  TSK_IMG_INFO* a_img_info,
  TSK_OFF_T a_off,
  char *a_buf,
  size_t a_len);

#ifdef __cplusplus
}
#endif
