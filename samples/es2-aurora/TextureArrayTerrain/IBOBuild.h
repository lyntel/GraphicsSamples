//----------------------------------------------------------------------------------
// File:        es2-aurora\TextureArrayTerrain/IBOBuild.h
// SDK Version: v3.00 
// Email:       gameworks@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2014-2015, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------------

/*
    t20/t30:
        optimal block_width = cache_entry_num - 3;
        prime_vtx cache = -1

    t114:
        optimal block_width = 64
        prime_vtx cache = 0
*/

#define HACK_NV_ASSERT NV_ASSERT


void gen_planar_quad_optimised_indices(
        int32_t patch_vtx_width,
        int32_t patch_vtx_height,
        int32_t *idx_num_ret,
        uint16_t **idx_ptr_ret,    // Caller must delete result
        int32_t block_vtx_width,
        bool warm_cache)
{
    HACK_NV_ASSERT(patch_vtx_width > 1);
    HACK_NV_ASSERT(block_vtx_width > 1);
    HACK_NV_ASSERT(patch_vtx_width <= patch_vtx_height);

    const int32_t block_num = ((patch_vtx_width - 1) + (block_vtx_width - 2)) / (block_vtx_width - 1);
    const int32_t final_block_vtx_width = patch_vtx_width - ((block_num - 1) * (block_vtx_width - 1));
    const int32_t tris_per_block = 2 * (block_vtx_width - 1) * (patch_vtx_height - 1);
    const int32_t tris_per_final_block = 2 * (final_block_vtx_width - 1) * (patch_vtx_height - 1);

    uint16_t *idx_ptr;
    int32_t idx_num;
    int32_t next_idx;
    int32_t i, j, k;

    idx_num = 3 * ((tris_per_block * (block_num - 1)) + tris_per_final_block);
    if (warm_cache)
        idx_num += 3 * ((block_vtx_width - 1) * (block_num - 1) + (final_block_vtx_width - 1));

    idx_ptr = new uint16_t[idx_num];
    HACK_NV_ASSERT(idx_ptr);

    /* blocks */
    next_idx = 0;
    for (i = 0; i < (patch_vtx_width - 1); i += (block_vtx_width - 1))
    {
        if (warm_cache)
        {
            /* prime vtx cache */
            for (k = i; (k < (i + block_vtx_width - 1)) && (k < (patch_vtx_width - 1)); k++)
            {
                idx_ptr[next_idx + 0] = (uint16_t)(k + 0);
                idx_ptr[next_idx + 1] = (uint16_t)(k + 1);
                idx_ptr[next_idx + 2] = (uint16_t)(k + 1);
                next_idx += 3;
            }
        }

        /* rows */
        for (j = 0; j < (patch_vtx_height - 1); j++)
        {
            /* tris */
            for (k = i; (k < (i + block_vtx_width - 1)) && (k < (patch_vtx_width - 1)); k++)
            {
                idx_ptr[next_idx + 0] = (uint16_t)((j + 0) * patch_vtx_width + (k + 0));
                idx_ptr[next_idx + 1] = (uint16_t)((j + 0) * patch_vtx_width + (k + 1));
                idx_ptr[next_idx + 2] = (uint16_t)((j + 1) * patch_vtx_width + (k + 0));

                idx_ptr[next_idx + 3] = (uint16_t)((j + 0) * patch_vtx_width + (k + 1));
                idx_ptr[next_idx + 4] = (uint16_t)((j + 1) * patch_vtx_width + (k + 1));
                idx_ptr[next_idx + 5] = (uint16_t)((j + 1) * patch_vtx_width + (k + 0));

                next_idx += 6;
            }
        }
    }

    HACK_NV_ASSERT(next_idx == idx_num);

    *idx_num_ret = idx_num;
    *idx_ptr_ret = idx_ptr;
}
