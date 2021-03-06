// Copyright(c) 2007-2016, Intel Corporation
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
/// @file uidrvMessage.h
/// @brief Defines the Message wrapper for the AAL Universal Device Driver.
/// @ingroup uAIA
/// @verbatim
/// Accelerator Abstraction Layer
///
/// AUTHOR: Joseph Grecco, Intel Corporation.
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 06/20/09       JG       Pulled from AALuAIA_UIDriverClient.h
/// 08/02/09       AC       Fixed a bug to initialize m_message to '0'
/// 06/17/10       AC       Fixed valgrind error to initialize m_payload to '0'
/// 03/12/2013     JG       Changed uidrvMessage to support link-less ioctlreq
/// 09/15/2015     JG       Removed message route and fixed up for 4.0@endverbatim
//****************************************************************************
#ifndef __AALSDK_AIASERVICE_UIDRVMESSAGE_H__
#define __AALSDK_AIASERVICE_UIDRVMESSAGE_H__
#include <aalsdk/AALTypes.h>
#include <aalsdk/AALBase.h>      // IBase
#include <aalsdk/kernel/ccipdriver.h> // uid_msgIDs_e, uid_errnum_e

#include <aalsdk/CUnCopyable.h>

BEGIN_NAMESPACE(AAL)

//==========================================================================
// Name: uidrvMessage
// Description: Wrapper for messages coming from IOCTLS
//==========================================================================
class AIASERVICE_API uidrvMessage : public CUnCopyable
{

public:
   uidrvMessage();
   virtual ~uidrvMessage();

   // size mutator (allocates m_payload)
   void size(btWSSize PayloadSize);
   // result_code mutator
   void result_code(uid_errnum_e e) {ASSERT(NULL != m_pmessage); m_pmessage->errcode = e; }

   operator struct ccipui_ioctlreq * ()           { ASSERT(NULL != m_pmessage); return m_pmessage;}
   struct ccipui_ioctlreq *   GetReqp()           { ASSERT(NULL != m_pmessage); return m_pmessage;}

   btHANDLE                  handle()      const { ASSERT(NULL != m_pmessage); return m_pmessage->handle;  }
   uid_msgIDs_e              id()          const { ASSERT(NULL != m_pmessage); return m_pmessage->id;      }
   uid_errnum_e              result_code() const { ASSERT(NULL != m_pmessage); return m_pmessage->errcode; }
   btWSSize                  size()        const { ASSERT(NULL != m_pmessage); return m_pmessage->size;    }
   btWSSize                  msgsize()     const { ASSERT(NULL != m_pmessage); return m_msgsize;    }
   stTransactionID_t const & tranID()      const { ASSERT(NULL != m_pmessage); return m_pmessage->tranID; }
   btObjectType              context()     const { ASSERT(NULL != m_pmessage);
                                                   return static_cast<btObjectType>(m_pmessage->context); }
   btVirtAddr                payload()     const;

protected:
   struct ccipui_ioctlreq *m_pmessage;
   btWSSize        m_msgsize;

}; // end of class uidrvMessage

END_NAMESPACE(AAL)

#endif // __AALSDK_AIASERVICE_UIDRVMESSAGE_H__

