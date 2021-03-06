// Copyright(c) 2014-2016, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//****************************************************************************
/// @file cci_mpf_csrs.h
/// @brief
/// @ingroup Utils
/// @verbatim
/// Accelerator Abstraction Layer Sample Application
///
///    This application is for example purposes only.
///    It is not intended to represent a model for developing commercially-deployable applications.
///    It is designed to show working examples of the AAL programming model and APIs.
///
/// AUTHORS: Sadruta Chandrashekar, Intel Corporation
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 05/26/2016     SC       Initial version (Taken from MPF repo)@endverbatim
//****************************************************************************
//
// CSR address offsets in MPF shims.
//
//   This file is included in both C and Verilog code and must be
//   syntactically correct in both.
//

//
// VTP feature -- {c8a2982f-ff96-42bf-a705-45727f501901}
//
typedef enum
{
    // VTP BBB feature header (read)
    CCI_MPF_VTP_CSR_DFH = 0,
    // BBB feature ID low (read)
    CCI_MPF_VTP_CSR_ID_L = 8,
    // BBB feature ID high (read)
    CCI_MPF_VTP_CSR_ID_H = 16,

    // Mode (4 bytes) (read/write)
    //   Bit 0:
    //      0 - Disabled (block memory traffic)
    //      1 - Enabled
    //   Bit 1:
    //      0 - Normal
    //      1 - Invalidate current FPGA-side translation cache.
    CCI_MPF_VTP_CSR_MODE = 24,

    // Page table physical address (line address) (write)
    CCI_MPF_VTP_CSR_PAGE_TABLE_PADDR = 32,

    // Statistics -- all 8 byte read-only CSRs
    CCI_MPF_VTP_CSR_STAT_4KB_TLB_NUM_HITS = 40,
    CCI_MPF_VTP_CSR_STAT_4KB_TLB_NUM_MISSES = 48,
    CCI_MPF_VTP_CSR_STAT_2MB_TLB_NUM_HITS = 56,
    CCI_MPF_VTP_CSR_STAT_2MB_TLB_NUM_MISSES = 64,
    CCI_MPF_VTP_CSR_STAT_PT_WALK_BUSY_CYCLES = 72,

    // Must be last
    CCI_MPF_VTP_CSR_SIZE = 80
}
t_cci_mpf_vtp_csr_offsets;


//
// Read responses ordering feature -- {4c9c96f4-65ba-4dd8-b383-c70ace57bfe4}
//
typedef enum
{
    // MPF write/read order BBB feature header (read)
    CCI_MPF_RSP_ORDER_CSR_DFH = 0,
    // BBB feature ID low (read)
    CCI_MPF_RSP_ORDER_CSR_ID_L = 8,
    // BBB feature ID high (read)
    CCI_MPF_RSP_ORDER_CSR_ID_H = 16,

    // eVC_VA to physical channel mapping configuration.  This is
    // mostly useful for experiments with physical mapping:
    //
    //   Bits 31-0:
    //      A vector of 16 t_cci_vc entries that are used to map
    //      read requests on eVC_VA to other other channels.  The
    //      vector is rotated on each read to VA, allowing the CSR
    //      to define a pattern of translations.
    //   Bit 32:
    //      When set, direct all reads to eVC_VA.  This overrides
    //      bits 31-0.
    //   Bit 33:
    //      When set, direct all writes to eVC_VA.
    //
    CCI_MPF_RSP_ORDER_CSR_C0TX_VA_VC_MAP = 24,

    // Must be last
    CCI_MPF_RSP_ORDER_CSR_SIZE = 32
}
t_cci_mpf_rsp_order_csr_offsets;


//
// Write ordering feature -- {56b06b48-9dd7-4004-a47e-0681b4207a6d}
//
typedef enum
{
    // MPF write/read order BBB feature header (read)
    CCI_MPF_WRO_CSR_DFH = 0,
    // BBB feature ID low (read)
    CCI_MPF_WRO_CSR_ID_L = 8,
    // BBB feature ID high (read)
    CCI_MPF_WRO_CSR_ID_H = 16,

    // Statistics -- all 8 byte read-only

    //   Total writes observed
    CCI_MPF_WRO_CSR_NUM_WR = 24,
    //   Total reads observed
    CCI_MPF_WRO_CSR_NUM_RD = 32,
    //   Total conflicting (blocked) writes
    CCI_MPF_WRO_CSR_NUM_WR_CONFLICTS = 40,
    //   Total conflicting (blocked) reads
    CCI_MPF_WRO_CSR_NUM_RD_CONFLICTS = 48,

    // Must be last
    CCI_MPF_WRO_CSR_SIZE = 56
}
t_cci_mpf_wro_csr_offsets;
