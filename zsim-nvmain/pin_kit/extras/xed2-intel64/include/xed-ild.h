/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2013 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */

/// @file xed-ild.h
/// instruction length decoder
    
#if !defined(_XED_ILD_H_)
# define _XED_ILD_H_
#include "xed-common-hdrs.h"
#include "xed-common-defs.h"
#include "xed-portability.h"
#include "xed-types.h"
#include "xed-decoded-inst.h"

#include "xed-operand-accessors.h"
    
/* ILD-related defines:
 *
 * XED_ILD - for building ILD module 
 * XED_ILD_CHECK  for using ILD module in decoding and checking 
 *                results against regular decode
 * XED_ILD_ONLY - for using only ILD for length decoding, without
 *                regular decoding. This is for performance measurements.
 */

static XED_INLINE xed_uint_t xed_modrm_mod(xed_uint8_t m) { return m>>6; }
static XED_INLINE xed_uint_t xed_modrm_reg(xed_uint8_t m) { return (m>>3)&7; }
static XED_INLINE xed_uint_t xed_modrm_rm(xed_uint8_t m) { return m&7; }
static XED_INLINE xed_uint_t xed_sib_scale(xed_uint8_t m) { return m>>6; }
static XED_INLINE xed_uint_t xed_sib_index(xed_uint8_t m) { return (m>>3)&7; }
static XED_INLINE xed_uint_t xed_sib_base(xed_uint8_t m) { return m&7; }
static XED_INLINE xed_uint_t bits2bytes(xed_uint_t bits) { return bits>>3; }


typedef enum {
    XED_ILD_MAP0=0,
    XED_ILD_MAP1=1,
    XED_ILD_MAP2=2,
    XED_ILD_MAP3=3,
    XED_ILD_MAPAMD=4,   /* amd 3dnow */
    XED_ILD_MAP_XOP8=5, /* amd xop */
    XED_ILD_MAP_XOP9=6, /* amd xop */
    XED_ILD_MAP_XOPA=7, /* amd xop */
    XED_ILD_INVALID_MAP
} xed_ild_map_enum_t;


#define XED_GRAMMAR_MODE_64 2
#define XED_GRAMMAR_MODE_32 1
#define XED_GRAMMAR_MODE_16 0


/// Initialize internal data structures of the ILD.
void xed_ild_init_decoder(void);

/// An instruction length decoder. This does not indicate if instructions
/// are valid or not. It only attempts to guess the overall length of the
/// instruction. The ild structure is modified.
///  @param d - should be initialized 
/// @return the length in bytes of the instruction, valid or not. 
///
/// @ingroup ILD
XED_DLL_EXPORT xed_uint_t
xed_instruction_length_decode(xed_decoded_inst_t* d);




/* Special getter for RM  */
static XED_INLINE xed_uint_t
xed_ild_get_rm(const xed_decoded_inst_t* ild) {
    /* Sometimes we don't have modrm, but grammar still
     * likes to use RM operand - in this case it is first
     * 3 bits of the opcode.
     */
    xed_uint8_t modrm;
    if (xed3_operand_get_has_modrm(ild))
        return xed3_operand_get_rm(ild);
    else 
        modrm = xed3_operand_get_nominal_opcode(ild);
        return xed_modrm_rm(modrm);
}

/* compressed operand getters */

static XED_INLINE xed_uint_t
xed3_operand_get_mod3(const xed_decoded_inst_t* ild) {
    return xed3_operand_get_mod(ild) == 3;
}

static XED_INLINE xed_uint_t
xed3_operand_get_rm4(const xed_decoded_inst_t* ild) {
    return xed3_operand_get_rm(ild) == 4;
}

static XED_INLINE xed_uint_t
xed3_operand_get_vexdest210_7(const xed_decoded_inst_t* ild) {
    return xed3_operand_get_vexdest210(ild) == 7; 
}

static XED_INLINE void 
xed3_check_set_ia_map(xed_decoded_inst_t* d, xed_uint8_t c4byte1){
    xed_uint8_t map = c4byte1 & 0x1F;
    if (map > XED_ILD_MAP3){
        xed3_operand_set_map(d,XED_ILD_INVALID_MAP);
    }
    else{
        xed3_operand_set_map(d,map);
    }
}

#endif

