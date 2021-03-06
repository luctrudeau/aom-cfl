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

#include "av1/common/onyxc_int.h"
#include "av1/common/entropymv.h"

// Integer pel reference mv threshold for use of high-precision 1/8 mv
#define COMPANDED_MVREF_THRESH 8

const aom_tree_index av1_mv_joint_tree[TREE_SIZE(MV_JOINTS)] = {
  -MV_JOINT_ZERO, 2, -MV_JOINT_HNZVZ, 4, -MV_JOINT_HZVNZ, -MV_JOINT_HNZVNZ
};

/* clang-format off */
const aom_tree_index av1_mv_class_tree[TREE_SIZE(MV_CLASSES)] = {
  -MV_CLASS_0, 2,
  -MV_CLASS_1, 4,
  6, 8,
  -MV_CLASS_2, -MV_CLASS_3,
  10, 12,
  -MV_CLASS_4, -MV_CLASS_5,
  -MV_CLASS_6, 14,
  16, 18,
  -MV_CLASS_7, -MV_CLASS_8,
  -MV_CLASS_9, -MV_CLASS_10,
};
/* clang-format on */

const aom_tree_index av1_mv_class0_tree[TREE_SIZE(CLASS0_SIZE)] = {
  -0, -1,
};

const aom_tree_index av1_mv_fp_tree[TREE_SIZE(MV_FP_SIZE)] = { -0, 2,  -1,
                                                               4,  -2, -3 };

static const nmv_context default_nmv_context = {
  { 32, 64, 96 },  // joints
#if CONFIG_EC_MULTISYMBOL
  { 0, 0, 0, 0 },  // joint_cdf is computed from joints in av1_init_mv_probs()
#endif
  { {
        // Vertical component
        128,                                                   // sign
        { 224, 144, 192, 168, 192, 176, 192, 198, 198, 245 },  // class
#if CONFIG_EC_MULTISYMBOL
        { 0 },  // class_cdf is computed from class in av1_init_mv_probs()
#endif
        { 216 },                                               // class0
        { 136, 140, 148, 160, 176, 192, 224, 234, 234, 240 },  // bits
        { { 128, 128, 64 }, { 96, 112, 64 } },                 // class0_fp
        { 64, 96, 64 },                                        // fp
#if CONFIG_EC_MULTISYMBOL
        { { 0 }, { 0 } },  // class0_fp_cdf is computed in av1_init_mv_probs()
        { 0 },             // fp_cdf is computed from fp in av1_init_mv_probs()
#endif
        160,  // class0_hp bit
        128,  // hp
    },
    {
        // Horizontal component
        128,                                                   // sign
        { 216, 128, 176, 160, 176, 176, 192, 198, 198, 208 },  // class
#if CONFIG_EC_MULTISYMBOL
        { 0 },  // class_cdf is computed from class in av1_init_mv_probs()
#endif
        { 208 },                                               // class0
        { 136, 140, 148, 160, 176, 192, 224, 234, 234, 240 },  // bits
        { { 128, 128, 64 }, { 96, 112, 64 } },                 // class0_fp
        { 64, 96, 64 },                                        // fp
#if CONFIG_EC_MULTISYMBOL
        { { 0 }, { 0 } },  // class0_fp_cdf is computed in av1_init_mv_probs()
        { 0 },             // fp_cdf is computed from fp in av1_init_mv_probs()
#endif
        160,  // class0_hp bit
        128,  // hp
    } },
};

static const uint8_t log_in_base_2[] = {
  0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10
};

static INLINE int mv_class_base(MV_CLASS_TYPE c) {
  return c ? CLASS0_SIZE << (c + 2) : 0;
}

MV_CLASS_TYPE av1_get_mv_class(int z, int *offset) {
  const MV_CLASS_TYPE c = (z >= CLASS0_SIZE * 4096)
                              ? MV_CLASS_10
                              : (MV_CLASS_TYPE)log_in_base_2[z >> 3];
  if (offset) *offset = z - mv_class_base(c);
  return c;
}

static void inc_mv_component(int v, nmv_component_counts *comp_counts, int incr,
                             int usehp) {
  int s, z, c, o, d, e, f;
  assert(v != 0); /* should not be zero */
  s = v < 0;
  comp_counts->sign[s] += incr;
  z = (s ? -v : v) - 1; /* magnitude - 1 */

  c = av1_get_mv_class(z, &o);
  comp_counts->classes[c] += incr;

  d = (o >> 3);     /* int mv data */
  f = (o >> 1) & 3; /* fractional pel mv data */
  e = (o & 1);      /* high precision mv data */

  if (c == MV_CLASS_0) {
    comp_counts->class0[d] += incr;
    comp_counts->class0_fp[d][f] += incr;
    comp_counts->class0_hp[e] += usehp * incr;
  } else {
    int i;
    int b = c + CLASS0_BITS - 1;  // number of bits
    for (i = 0; i < b; ++i) comp_counts->bits[i][((d >> i) & 1)] += incr;
    comp_counts->fp[f] += incr;
    comp_counts->hp[e] += usehp * incr;
  }
}

void av1_inc_mv(const MV *mv, nmv_context_counts *counts, const int usehp) {
  if (counts != NULL) {
    const MV_JOINT_TYPE j = av1_get_mv_joint(mv);
    ++counts->joints[j];

    if (mv_joint_vertical(j))
      inc_mv_component(mv->row, &counts->comps[0], 1, usehp);

    if (mv_joint_horizontal(j))
      inc_mv_component(mv->col, &counts->comps[1], 1, usehp);
  }
}

void av1_adapt_mv_probs(AV1_COMMON *cm, int allow_hp) {
  int i, j;
#if CONFIG_REF_MV
  int idx;
  for (idx = 0; idx < NMV_CONTEXTS; ++idx) {
    nmv_context *fc = &cm->fc->nmvc[idx];
    const nmv_context *pre_fc =
        &cm->frame_contexts[cm->frame_context_idx].nmvc[idx];
    const nmv_context_counts *counts = &cm->counts.mv[idx];

    aom_tree_merge_probs(av1_mv_joint_tree, pre_fc->joints, counts->joints,
                         fc->joints);

    for (i = 0; i < 2; ++i) {
      nmv_component *comp = &fc->comps[i];
      const nmv_component *pre_comp = &pre_fc->comps[i];
      const nmv_component_counts *c = &counts->comps[i];

      comp->sign = mode_mv_merge_probs(pre_comp->sign, c->sign);
      aom_tree_merge_probs(av1_mv_class_tree, pre_comp->classes, c->classes,
                           comp->classes);
      aom_tree_merge_probs(av1_mv_class0_tree, pre_comp->class0, c->class0,
                           comp->class0);

      for (j = 0; j < MV_OFFSET_BITS; ++j)
        comp->bits[j] = mode_mv_merge_probs(pre_comp->bits[j], c->bits[j]);

      for (j = 0; j < CLASS0_SIZE; ++j)
        aom_tree_merge_probs(av1_mv_fp_tree, pre_comp->class0_fp[j],
                             c->class0_fp[j], comp->class0_fp[j]);

      aom_tree_merge_probs(av1_mv_fp_tree, pre_comp->fp, c->fp, comp->fp);

      if (allow_hp) {
        comp->class0_hp =
            mode_mv_merge_probs(pre_comp->class0_hp, c->class0_hp);
        comp->hp = mode_mv_merge_probs(pre_comp->hp, c->hp);
      }
    }
  }
#else
  nmv_context *fc = &cm->fc->nmvc;
  const nmv_context *pre_fc = &cm->frame_contexts[cm->frame_context_idx].nmvc;
  const nmv_context_counts *counts = &cm->counts.mv;

  aom_tree_merge_probs(av1_mv_joint_tree, pre_fc->joints, counts->joints,
                       fc->joints);

  for (i = 0; i < 2; ++i) {
    nmv_component *comp = &fc->comps[i];
    const nmv_component *pre_comp = &pre_fc->comps[i];
    const nmv_component_counts *c = &counts->comps[i];

    comp->sign = mode_mv_merge_probs(pre_comp->sign, c->sign);
    aom_tree_merge_probs(av1_mv_class_tree, pre_comp->classes, c->classes,
                         comp->classes);
    aom_tree_merge_probs(av1_mv_class0_tree, pre_comp->class0, c->class0,
                         comp->class0);

    for (j = 0; j < MV_OFFSET_BITS; ++j)
      comp->bits[j] = mode_mv_merge_probs(pre_comp->bits[j], c->bits[j]);

    for (j = 0; j < CLASS0_SIZE; ++j)
      aom_tree_merge_probs(av1_mv_fp_tree, pre_comp->class0_fp[j],
                           c->class0_fp[j], comp->class0_fp[j]);

    aom_tree_merge_probs(av1_mv_fp_tree, pre_comp->fp, c->fp, comp->fp);

    if (allow_hp) {
      comp->class0_hp = mode_mv_merge_probs(pre_comp->class0_hp, c->class0_hp);
      comp->hp = mode_mv_merge_probs(pre_comp->hp, c->hp);
    }
  }
#endif
}

#if CONFIG_EC_MULTISYMBOL
void av1_set_mv_cdfs(nmv_context *ctx) {
  int i;
  int j;
  av1_tree_to_cdf(av1_mv_joint_tree, ctx->joints, ctx->joint_cdf);

  for (i = 0; i < 2; ++i) {
    nmv_component *const comp_ctx = &ctx->comps[i];
    av1_tree_to_cdf(av1_mv_class_tree, comp_ctx->classes, comp_ctx->class_cdf);

    for (j = 0; j < CLASS0_SIZE; ++j) {
      av1_tree_to_cdf(av1_mv_fp_tree, comp_ctx->class0_fp[j],
                      comp_ctx->class0_fp_cdf[j]);
    }
    av1_tree_to_cdf(av1_mv_fp_tree, comp_ctx->fp, comp_ctx->fp_cdf);
  }
}
#endif

void av1_init_mv_probs(AV1_COMMON *cm) {
#if CONFIG_REF_MV
  int i;
  for (i = 0; i < NMV_CONTEXTS; ++i) cm->fc->nmvc[i] = default_nmv_context;
#else
  cm->fc->nmvc = default_nmv_context;
#if CONFIG_EC_MULTISYMBOL
  av1_set_mv_cdfs(&cm->fc->nmvc);
#endif
#endif
}
