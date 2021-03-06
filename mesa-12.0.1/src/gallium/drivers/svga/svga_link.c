/*/
 * Copyright 2013 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include "svga_context.h"
#include "svga_link.h"

#include "tgsi/tgsi_strings.h"


#define INVALID_INDEX 255


/**
 * Examine input and output shaders info to link outputs from the
 * output shader to inputs from the input shader.
 * Basically, we'll remap input shader's input slots to new numbers
 * based on semantic name/index of the outputs from the output shader.
 */
void
svga_link_shaders(const struct tgsi_shader_info *outshader_info,
                  const struct tgsi_shader_info *inshader_info,
                  struct shader_linkage *linkage)
{
   unsigned i, free_slot;

   for (i = 0; i < ARRAY_SIZE(linkage->input_map); i++) {
      linkage->input_map[i] = INVALID_INDEX;
   }

   /* Assign input slots for input shader inputs.
    * Basically, we want to use the same index for the output shader's outputs
    * and the input shader's inputs that should be linked together.
    * We'll modify the input shader's inputs to match the output shader.
    */
   assert(inshader_info->num_inputs <=
          ARRAY_SIZE(inshader_info->input_semantic_name));

   /* free register index that can be used for built-in varyings */
   free_slot = outshader_info->num_outputs + 1;

   for (i = 0; i < inshader_info->num_inputs; i++) {
      unsigned sem_name = inshader_info->input_semantic_name[i];
      unsigned sem_index = inshader_info->input_semantic_index[i];
      unsigned j;
      /**
       * Get the clip distance inputs from the output shader's
       * clip distance shadow copy.
       */
      if (sem_name == TGSI_SEMANTIC_CLIPDIST) {
         linkage->input_map[i] = outshader_info->num_outputs + 1 + sem_index;
         /* make sure free_slot includes this extra output */
         free_slot = MAX2(free_slot, linkage->input_map[i] + 1);
      }
      else {
         /* search output shader outputs for same item */
         for (j = 0; j < outshader_info->num_outputs; j++) {
            assert(j < ARRAY_SIZE(outshader_info->output_semantic_name));
            if (outshader_info->output_semantic_name[j] == sem_name &&
                outshader_info->output_semantic_index[j] == sem_index) {
               linkage->input_map[i] = j;
               break;
            }
         }
      }
   }

   linkage->num_inputs = inshader_info->num_inputs;

   /* Things like the front-face register are handled here */
   for (i = 0; i < inshader_info->num_inputs; i++) {
      if (linkage->input_map[i] == INVALID_INDEX) {
         unsigned j = free_slot++;
         linkage->input_map[i] = j;
      }
   }

   /* Debug */
   if (0) {
      unsigned reg = 0;
      for (i = 0; i < linkage->num_inputs; i++) {

         assert(linkage->input_map[i] != INVALID_INDEX);

         debug_printf("input shader input[%d] slot %u  %s %u %s\n",
                      i,
                      linkage->input_map[i],
                      tgsi_semantic_names[inshader_info->input_semantic_name[i]],
                      inshader_info->input_semantic_index[i],
                      tgsi_interpolate_names[inshader_info->input_interpolate[i]]);

         /* make sure no repeating register index */
         if (reg & 1 << linkage->input_map[i]) {
            assert(0);
         }
         reg |= 1 << linkage->input_map[i];
      }
   }
}
