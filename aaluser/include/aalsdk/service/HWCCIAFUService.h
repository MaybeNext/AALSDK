// Copyright (c) 2014-2015, Intel Corporation
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
/// @file HWCCIAFUService.h
/// @brief AAL Service Module definitions for Hardware CCI AFU
/// @ingroup HWCCIAFU
/// @verbatim
/// Intel(R) QuickAssist Technology Accelerator Abstraction Layer
///
/// AUTHORS: Tim Whisonant, Intel Corporation
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 07/18/2014     TSW      Initial version.@endverbatim
//****************************************************************************
#ifndef __SERVICE_HWCCIAFUSERVICE_H__
#define __SERVICE_HWCCIAFUSERVICE_H__
#include <aalsdk/osal/OSServiceModule.h>
#include <aalsdk/INTCDefs.h>

/// @addtogroup HWCCIAFU
/// @{

#if defined ( __AAL_WINDOWS__ )
# ifdef HWCCIAFU_EXPORTS
#    define HWCCIAFU_API __declspec(dllexport)
# else
#    define HWCCIAFU_API __declspec(dllimport)
# endif // HWCCIAFU_EXPORTS
#else
# ifndef __declspec
#    define __declspec(x)
# endif // __declspec
# define HWCCIAFU_API    __declspec(0)
#endif // __AAL_WINDOWS__

#define HWCCIAFU_SVC_MOD         "libHWCCIAFU" AAL_SVC_MOD_EXT
#define HWCCIAFU_SVC_ENTRY_POINT "libHWCCIAFU" AAL_SVC_MOD_ENTRY_SUFFIX

#define HWCCIAFU_BEGIN_SVC_MOD(__svcfactory) AAL_BEGIN_SVC_MOD(__svcfactory, libHWCCIAFU, HWCCIAFU_API, HWCCIAFU_VERSION, HWCCIAFU_VERSION_CURRENT, HWCCIAFU_VERSION_REVISION, HWCCIAFU_VERSION_AGE)
#define HWCCIAFU_END_SVC_MOD()               AAL_END_SVC_MOD()

AAL_DECLARE_SVC_MOD(libHWCCIAFU, HWCCIAFU_API)

/// CCI Hardware AFU interface ID.
#define iidHWCCIAFU __INTC_IID(INTC_sysSampleAFU, 0x0008)

// 64-bit  AFU-ID: 00000000-0000-0000-9AEF-FE5F84570612
// 128-bit AFU-ID: C000C966-0D82-4272-9AEF-FE5F84570612

#define HWCCIAFU_MANIFEST \
"\t9 16 AAL_keyRegAFU_ID\n \
\t\t9 36 C000C966-0D82-4272-9AEF-FE5F84570612\n \
9 20 ConfigRecordIncluded\n \
\t10\n \
\t\t9 16 AAL_keyRegAFU_ID\n \
\t\t\t9 36 C000C966-0D82-4272-9AEF-FE5F84570612\n \
\t\t9 13 AIAExecutable\n \
\t\t\t9 10 libAASUAIA\n \
\t\t9 17 ServiceExecutable\n \
\t\t\t9 11 libHWCCIAFU\n \
9 29 ---- End of embedded NVS ----\n \
\t9999\n \
9 11 ServiceName\n \
\t9 8 HWCCIAFU\n"

/// @}

#endif // __SERVICE_HWCCIAFUSERVICE_H__

