/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include "third_party/googletest/src/include/gtest/gtest.h"

#include "./aom_dsp_rtcd.h"
#include "test/acm_random.h"
#include "av1/common/filter.h"
#include "av1/common/convolve.h"
#include "aom_dsp/aom_dsp_common.h"
#include "aom_ports/mem.h"

using libaom_test::ACMRandom;

namespace {
TEST(AV1ConvolveTest, av1_convolve8) {
  ACMRandom rnd(ACMRandom::DeterministicSeed());
  InterpFilter interp_filter = EIGHTTAP;
  InterpFilterParams filter_params = get_interp_filter_params(interp_filter);
  ptrdiff_t filter_size = filter_params.taps;
  int filter_center = filter_size / 2 - 1;
  uint8_t src[12 * 12];
  int src_stride = filter_size;
  uint8_t dst[1] = { 0 };
  uint8_t dst1[1] = { 0 };
  int dst_stride = 1;
  int x_step_q4 = 16;
  int y_step_q4 = 16;
  int subpel_x_q4 = 3;
  int subpel_y_q4 = 2;
  int avg = 0;

  int w = 1;
  int h = 1;

  for (int i = 0; i < filter_size * filter_size; i++) {
    src[i] = rnd.Rand16() % (1 << 8);
  }

  av1_convolve(src + src_stride * filter_center + filter_center, src_stride,
               dst, dst_stride, w, h, &interp_filter, subpel_x_q4, x_step_q4,
               subpel_y_q4, y_step_q4, avg);

  const int16_t *x_filter =
      get_interp_filter_subpel_kernel(filter_params, subpel_x_q4);
  const int16_t *y_filter =
      get_interp_filter_subpel_kernel(filter_params, subpel_y_q4);

  aom_convolve8_c(src + src_stride * filter_center + filter_center, src_stride,
                  dst1, dst_stride, x_filter, 16, y_filter, 16, w, h);
  EXPECT_EQ(dst[0], dst1[0]);
}
TEST(AV1ConvolveTest, av1_convolve) {
  ACMRandom rnd(ACMRandom::DeterministicSeed());
  InterpFilter interp_filter = EIGHTTAP;
  InterpFilterParams filter_params = get_interp_filter_params(interp_filter);
  ptrdiff_t filter_size = filter_params.taps;
  int filter_center = filter_size / 2 - 1;
  uint8_t src[12 * 12];
  int src_stride = filter_size;
  uint8_t dst[1] = { 0 };
  int dst_stride = 1;
  int x_step_q4 = 16;
  int y_step_q4 = 16;
  int avg = 0;
  int w = 1;
  int h = 1;

  int subpel_x_q4;
  int subpel_y_q4;

  for (int i = 0; i < filter_size * filter_size; i++) {
    src[i] = rnd.Rand16() % (1 << 8);
  }

  for (subpel_x_q4 = 0; subpel_x_q4 < 16; subpel_x_q4++) {
    for (subpel_y_q4 = 0; subpel_y_q4 < 16; subpel_y_q4++) {
      av1_convolve(src + src_stride * filter_center + filter_center, src_stride,
                   dst, dst_stride, w, h, &interp_filter, subpel_x_q4,
                   x_step_q4, subpel_y_q4, y_step_q4, avg);

      const int16_t *x_filter =
          get_interp_filter_subpel_kernel(filter_params, subpel_x_q4);
      const int16_t *y_filter =
          get_interp_filter_subpel_kernel(filter_params, subpel_y_q4);

      int temp[12];
      int dst_ref = 0;
      for (int r = 0; r < filter_size; r++) {
        temp[r] = 0;
        for (int c = 0; c < filter_size; c++) {
          temp[r] += x_filter[c] * src[r * filter_size + c];
        }
        temp[r] = clip_pixel(ROUND_POWER_OF_TWO(temp[r], FILTER_BITS));
        dst_ref += temp[r] * y_filter[r];
      }
      dst_ref = clip_pixel(ROUND_POWER_OF_TWO(dst_ref, FILTER_BITS));
      EXPECT_EQ(dst[0], dst_ref);
    }
  }
}

TEST(AV1ConvolveTest, av1_convolve_avg) {
  ACMRandom rnd(ACMRandom::DeterministicSeed());
  InterpFilter interp_filter = EIGHTTAP;
  InterpFilterParams filter_params = get_interp_filter_params(interp_filter);
  ptrdiff_t filter_size = filter_params.taps;
  int filter_center = filter_size / 2 - 1;
  uint8_t src0[12 * 12];
  uint8_t src1[12 * 12];
  int src_stride = filter_size;
  uint8_t dst0[1] = { 0 };
  uint8_t dst1[1] = { 0 };
  uint8_t dst[1] = { 0 };
  int dst_stride = 1;
  int x_step_q4 = 16;
  int y_step_q4 = 16;
  int avg = 0;

  int w = 1;
  int h = 1;

  int subpel_x_q4;
  int subpel_y_q4;

  for (int i = 0; i < filter_size * filter_size; i++) {
    src0[i] = rnd.Rand16() % (1 << 8);
    src1[i] = rnd.Rand16() % (1 << 8);
  }

  int offset = filter_size * filter_center + filter_center;

  for (subpel_x_q4 = 0; subpel_x_q4 < 16; subpel_x_q4++) {
    for (subpel_y_q4 = 0; subpel_y_q4 < 16; subpel_y_q4++) {
      avg = 0;
      av1_convolve(src0 + offset, src_stride, dst0, dst_stride, w, h,
                   &interp_filter, subpel_x_q4, x_step_q4, subpel_y_q4,
                   y_step_q4, avg);
      avg = 0;
      av1_convolve(src1 + offset, src_stride, dst1, dst_stride, w, h,
                   &interp_filter, subpel_x_q4, x_step_q4, subpel_y_q4,
                   y_step_q4, avg);
      avg = 0;
      av1_convolve(src0 + offset, src_stride, dst, dst_stride, w, h,
                   &interp_filter, subpel_x_q4, x_step_q4, subpel_y_q4,
                   y_step_q4, avg);
      avg = 1;
      av1_convolve(src1 + offset, src_stride, dst, dst_stride, w, h,
                   &interp_filter, subpel_x_q4, x_step_q4, subpel_y_q4,
                   y_step_q4, avg);

      EXPECT_EQ(dst[0], ROUND_POWER_OF_TWO(dst0[0] + dst1[0], 1));
    }
  }
}

#if CONFIG_AOM_HIGHBITDEPTH
TEST(AV1ConvolveTest, av1_highbd_convolve) {
  ACMRandom rnd(ACMRandom::DeterministicSeed());
  InterpFilter interp_filter = EIGHTTAP;
  InterpFilterParams filter_params = get_interp_filter_params(interp_filter);
  ptrdiff_t filter_size = filter_params.taps;
  int filter_center = filter_size / 2 - 1;
  uint16_t src[12 * 12];
  int src_stride = filter_size;
  uint16_t dst[1] = { 0 };
  int dst_stride = 1;
  int x_step_q4 = 16;
  int y_step_q4 = 16;
  int avg = 0;
  int bd = 10;
  int w = 1;
  int h = 1;

  int subpel_x_q4;
  int subpel_y_q4;

  for (int i = 0; i < filter_size * filter_size; i++) {
    src[i] = rnd.Rand16() % (1 << bd);
  }

  for (subpel_x_q4 = 0; subpel_x_q4 < 16; subpel_x_q4++) {
    for (subpel_y_q4 = 0; subpel_y_q4 < 16; subpel_y_q4++) {
      av1_highbd_convolve(
          CONVERT_TO_BYTEPTR(src) + src_stride * filter_center + filter_center,
          src_stride, CONVERT_TO_BYTEPTR(dst), dst_stride, w, h, &interp_filter,
          subpel_x_q4, x_step_q4, subpel_y_q4, y_step_q4, avg, bd);

      const int16_t *x_filter =
          get_interp_filter_subpel_kernel(filter_params, subpel_x_q4);
      const int16_t *y_filter =
          get_interp_filter_subpel_kernel(filter_params, subpel_y_q4);

      int temp[12];
      int dst_ref = 0;
      for (int r = 0; r < filter_size; r++) {
        temp[r] = 0;
        for (int c = 0; c < filter_size; c++) {
          temp[r] += x_filter[c] * src[r * filter_size + c];
        }
        temp[r] =
            clip_pixel_highbd(ROUND_POWER_OF_TWO(temp[r], FILTER_BITS), bd);
        dst_ref += temp[r] * y_filter[r];
      }
      dst_ref = clip_pixel_highbd(ROUND_POWER_OF_TWO(dst_ref, FILTER_BITS), bd);
      EXPECT_EQ(dst[0], dst_ref);
    }
  }
}

TEST(AV1ConvolveTest, av1_highbd_convolve_avg) {
  ACMRandom rnd(ACMRandom::DeterministicSeed());
  InterpFilter interp_filter = EIGHTTAP;
  InterpFilterParams filter_params = get_interp_filter_params(interp_filter);
  ptrdiff_t filter_size = filter_params.taps;
  int filter_center = filter_size / 2 - 1;
  uint16_t src0[12 * 12];
  uint16_t src1[12 * 12];
  int src_stride = filter_size;
  uint16_t dst0[1] = { 0 };
  uint16_t dst1[1] = { 0 };
  uint16_t dst[1] = { 0 };
  int dst_stride = 1;
  int x_step_q4 = 16;
  int y_step_q4 = 16;
  int avg = 0;
  int bd = 10;

  int w = 1;
  int h = 1;

  int subpel_x_q4;
  int subpel_y_q4;

  for (int i = 0; i < filter_size * filter_size; i++) {
    src0[i] = rnd.Rand16() % (1 << bd);
    src1[i] = rnd.Rand16() % (1 << bd);
  }

  for (subpel_x_q4 = 0; subpel_x_q4 < 16; subpel_x_q4++) {
    for (subpel_y_q4 = 0; subpel_y_q4 < 16; subpel_y_q4++) {
      int offset = filter_size * filter_center + filter_center;

      avg = 0;
      av1_highbd_convolve(CONVERT_TO_BYTEPTR(src0) + offset, src_stride,
                          CONVERT_TO_BYTEPTR(dst0), dst_stride, w, h,
                          &interp_filter, subpel_x_q4, x_step_q4, subpel_y_q4,
                          y_step_q4, avg, bd);
      avg = 0;
      av1_highbd_convolve(CONVERT_TO_BYTEPTR(src1) + offset, src_stride,
                          CONVERT_TO_BYTEPTR(dst1), dst_stride, w, h,
                          &interp_filter, subpel_x_q4, x_step_q4, subpel_y_q4,
                          y_step_q4, avg, bd);

      avg = 0;
      av1_highbd_convolve(CONVERT_TO_BYTEPTR(src0) + offset, src_stride,
                          CONVERT_TO_BYTEPTR(dst), dst_stride, w, h,
                          &interp_filter, subpel_x_q4, x_step_q4, subpel_y_q4,
                          y_step_q4, avg, bd);
      avg = 1;
      av1_highbd_convolve(CONVERT_TO_BYTEPTR(src1) + offset, src_stride,
                          CONVERT_TO_BYTEPTR(dst), dst_stride, w, h,
                          &interp_filter, subpel_x_q4, x_step_q4, subpel_y_q4,
                          y_step_q4, avg, bd);

      EXPECT_EQ(dst[0], ROUND_POWER_OF_TWO(dst0[0] + dst1[0], 1));
    }
  }
}
#endif  // CONFIG_AOM_HIGHBITDEPTH
}  // namespace
