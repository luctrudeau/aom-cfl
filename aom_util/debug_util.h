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

#ifndef AOM_UTIL_DEBUG_UTIL_H_
#define AOM_UTIL_DEBUG_UTIL_H_

#include "./aom_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This is a debug tool used to detect bitstream error. On encoder side, it
 * pushes each bit and probability into a queue before the bit is written into
 * the Arithmetic coder. On decoder side, whenever a bit is read out from the
 * Arithmetic coder, it pops out the reference bit and probability from the
 * queue as well. If the two results do not match, this debug tool will report
 * an error.  This tool can be used to pin down the bitstream error precisely.
 * By combining gdb's backtrace method, we can detect which module causes the
 * bitstream error. */
int bitstream_queue_get_write(void);
int bitstream_queue_get_read(void);
void bitstream_queue_record_write(void);
void bitstream_queue_reset_write(void);
void bitstream_queue_pop(int *result, int *prob);
void bitstream_queue_push(int result, int prob);
void bitstream_queue_set_skip_write(int skip);
void bitstream_queue_set_skip_read(int skip);
void bitstream_queue_set_frame_write(int frame_idx);
int bitstream_queue_get_frame_write(void);
void bitstream_queue_set_frame_read(int frame_idx);
int bitstream_queue_get_frame_read(void);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AOM_UTIL_DEBUG_UTIL_H_
