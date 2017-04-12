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

#ifndef AV1_COMMON_RECONINTRA_H_
#define AV1_COMMON_RECONINTRA_H_

#include "aom/aom_integer.h"
#include "av1/common/blockd.h"

#ifdef __cplusplus
extern "C" {
#endif

void av1_init_intra_predictors(void);

void av1_predict_intra_block(const MACROBLOCKD *xd, int bw, int bh,
                             BLOCK_SIZE bsize, PREDICTION_MODE mode,
                             const uint8_t *ref, int ref_stride, uint8_t *dst,
                             int dst_stride, int aoff, int loff, int plane);

#if CONFIG_EXT_INTER
// Mapping of interintra to intra mode for use in the intra component
static const PREDICTION_MODE interintra_to_intra_mode[INTERINTRA_MODES] = {
  DC_PRED,   V_PRED,    H_PRED,    D45_PRED, D135_PRED,
  D117_PRED, D153_PRED, D207_PRED, D63_PRED, TM_PRED
};

// Mapping of intra mode to the interintra mode
static const INTERINTRA_MODE intra_to_interintra_mode[INTRA_MODES] = {
  II_DC_PRED,   II_V_PRED,    II_H_PRED,    II_D45_PRED, II_D135_PRED,
  II_D117_PRED, II_D153_PRED, II_D207_PRED, II_D63_PRED,
#if CONFIG_ALT_INTRA
  II_DC_PRED,  // Note: Filler value, as there's no II_SMOOTH_PRED.
#endif         // CONFIG_ALT_INTRA
  II_TM_PRED
};
#endif  // CONFIG_EXT_INTER
#ifdef __cplusplus
}  // extern "C"
#endif

#if CONFIG_FILTER_INTRA
#define FILTER_INTRA_PREC_BITS 10
extern int av1_filter_intra_taps_4[TX_SIZES][INTRA_MODES][4];
#endif  // CONFIG_FILTER_INTRA

#if CONFIG_EXT_INTRA
static INLINE int av1_is_directional_mode(PREDICTION_MODE mode,
                                          BLOCK_SIZE bsize) {
  return mode != DC_PRED && mode != TM_PRED &&
#if CONFIG_ALT_INTRA
         mode != SMOOTH_PRED &&
#endif  // CONFIG_ALT_INTRA
         bsize >= BLOCK_8X8;
}
#endif  // CONFIG_EXT_INTRA

#if CONFIG_CFL
static const double cfl_alpha_codes[CFL_MAX_ALPHA_IND][2] = {
  // Notes:
  //   * First code MUST be (0.0, 0.0)
  //   * No sign bit is signaled for values of 0.0

  // Optimized using barrbrain's secret sauce.
  /*
  { 0.0, 0.0 },           { 0.040979, 0.033698 }, { 0.046883, 0.119568 },
  { 0.122612, 0.041332 }, { 0.07343, 0.267272 },  { 0.247968, 0.053005 },
  { 0.169956, 0.451542 }, { 0.492068, 0.19885 },  { 0.204593, 0.183029 },
  { 1.003407, 0.480246 }, { 0.277097, 0.712143 }, { 0.56754, 1.067072 },
  { 2.495618, 0.212118 }, { 1.738761, 0.077004 }, { 3.095779, 0.474077 },
  { 1.194377, 1.949127 }
  */
  // barrbrain's subset 3 training
  /*
  { 0.0, 0.0 },           { 0.042698, 0.043504 }, { 0.120795, 0.056032 },
  { 0.061485, 0.15329 },  { 0.22606, 0.058298 },  { 0.201798, 0.185862 },
  { 0.102267, 0.332564 }, { 0.378463, 0.09758 },  { 0.346785, 0.322548 },
  { 0.599929, 0.175545 }, { 0.20524, 0.604609 },  { 0.555772, 0.543228 },
  { 0.48154, 1.155061 },  { 1.188442, 0.447334 }, { 2.432934, 0.676077 },
  { 1.373512, 2.382514 }
  */
  // 1D quant (from notebook)
  { 0.0, 0.0 },
  //{ 0.03196339, 0.1298463 },
  //{ 0.03196339, 0.32787086 },
  //{ 0.03196339, 0.87551493 },
  { 0.0, 0.1298463 },
  { 0.0, 0.32787086 },
  { 0.0, 0.87551493 },
  // { 0.11864264, 0.03513512 },
  { 0.11864264, 0.0 },
  { 0.11864264, 0.1298463 },
  { 0.11864264, 0.32787086 },
  { 0.11864264, 0.87551493 },
  // { 0.27456574, 0.03513512 },
  { 0.27456574, 0.0 },
  { 0.27456574, 0.1298463 },
  { 0.27456574, 0.32787086 },
  { 0.27456574, 0.87551493 },
  // { 0.67408152, 0.03513512 },
  { 0.67408152, 0.0 },
  { 0.67408152, 0.1298463 },
  { 0.67408152, 0.32787086 },
  { 0.67408152, 0.87551493 }
  // 2D quant (from notebook)
  /*
  { 0., 0. },
  { 0.06149382, 0.11290525 },
  { 0.08599878, 0.25676731 },
  { 0.40249788, 1.00118213 },
  //{ 0.12605572, 0.04967548 },
  { 0.12605572, 0.0 },  // Cheating!
  { 0.17625133, 0.14352254 },
  { 0.17687939, 0.41829049 },
  { 0.21390173, 0.6476021 },
  //{ 0.27298683, 0.06685685 },
  { 0.27298683, 0.0 },  // Cheating!
  { 0.31192775, 0.2399006 },
  { 0.49597766, 0.52877945 },
  { 0.51336979, 1.81650332 },
  { 0.48182567, 0.09887955 },
  { 0.65410737, 0.28417644 },
  { 1.27372142, 0.62908472 },
  { 2.63233951, 3.62685875 }
  */
  // Mimic 1D Quant (to match previous results)
  /*
  { 0.0, 0.0 },           { 0.0, 0.122874 },      { 0.0, 0.286103 },
  { 0.0, 0.854692 },      { 0.122874, 0.0 },      { 0.122874, 0.122874 },
  { 0.122874, 0.286103 }, { 0.122874, 0.854692 }, { 0.286103, 0.0 },
  { 0.286103, 0.122874 }, { 0.286103, 0.286103 }, { 0.286103, 0.854692 },
  { 0.854692, 0.0 },      { 0.854692, 0.122874 }, { 0.854692, 0.286103 },
  { 0.854692, 0.854692 }
  */
};

void cfl_dc_pred(MACROBLOCKD *const xd, CFL_CTX *const cfl,
                 BLOCK_SIZE plane_bsize, TX_SIZE tx_size);

void cfl_predict_block(const CFL_CTX *const cfl, uint8_t *const dst,
                       int dst_stride, int row, int col, TX_SIZE tx_size,
                       int alpha_ind, int alpha_sign, int plane);

int cfl_load(const CFL_CTX *const cfl, uint8_t *const output, int output_stride,
             int row, int col, int tx_block_width, int plane);

void cfl_store(CFL_CTX *const cfl, uint8_t *const input, int input_stride,
               int row, int col, int tx_blk_size);
#endif
#endif  // AV1_COMMON_RECONINTRA_H_
