//******************************************************************************
// Part of the Intel(R) QuickAssist Technology Accelerator Abstraction Layer
//
// This  file  is  provided  under  a  dual BSD/GPLv2  license.  When using or
//         redistributing this file, you may do so under either license.
//
//                            GPL LICENSE SUMMARY
//
//  Copyright(c) 2015, Intel Corporation.
//
//  This program  is  free software;  you  can redistribute it  and/or  modify
//  it  under  the  terms of  version 2 of  the GNU General Public License  as
//  published by the Free Software Foundation.
//
//  This  program  is distributed  in the  hope that it  will  be useful,  but
//  WITHOUT   ANY   WARRANTY;   without   even  the   implied   warranty    of
//  MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the   GNU
//  General Public License for more details.
//
//  The  full  GNU  General Public License is  included in  this  distribution
//  in the file called README.GPLV2-LICENSE.TXT.
//
//  Contact Information:
//  Henry Mitchel, henry.mitchel at intel.com
//  77 Reed Rd., Hudson, MA  01749
//
//                                BSD LICENSE
//
//  Copyright(c) 2015, Intel Corporation.
//
//  Redistribution and  use  in source  and  binary  forms,  with  or  without
//  modification,  are   permitted  provided  that  the  following  conditions
//  are met:
//
//    * Redistributions  of  source  code  must  retain  the  above  copyright
//      notice, this list of conditions and the following disclaimer.
//    * Redistributions in  binary form  must  reproduce  the  above copyright
//      notice,  this  list of  conditions  and  the  following disclaimer  in
//      the   documentation   and/or   other   materials   provided  with  the
//      distribution.
//    * Neither   the  name   of  Intel  Corporation  nor  the  names  of  its
//      contributors  may  be  used  to  endorse  or promote  products derived
//      from this software without specific prior written permission.
//
//  THIS  SOFTWARE  IS  PROVIDED  BY  THE  COPYRIGHT HOLDERS  AND CONTRIBUTORS
//  "AS IS"  AND  ANY  EXPRESS  OR  IMPLIED  WARRANTIES,  INCLUDING,  BUT  NOT
//  LIMITED  TO, THE  IMPLIED WARRANTIES OF  MERCHANTABILITY  AND FITNESS  FOR
//  A  PARTICULAR  PURPOSE  ARE  DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,
//  SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL   DAMAGES  (INCLUDING,   BUT   NOT
//  LIMITED  TO,  PROCUREMENT  OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF USE,
//  DATA,  OR PROFITS;  OR BUSINESS INTERRUPTION)  HOWEVER  CAUSED  AND ON ANY
//  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT LIABILITY,  OR TORT
//  (INCLUDING  NEGLIGENCE  OR OTHERWISE) ARISING  IN ANY WAY  OUT  OF THE USE
//  OF  THIS  SOFTWARE, EVEN IF ADVISED  OF  THE  POSSIBILITY  OF SUCH DAMAGE.
//******************************************************************************
//****************************************************************************
/// @file ccip_port.h
/// @brief  Definitions for CCI Port.
/// @ingroup aalkernel_ccip
/// @verbatim
//        FILE: ccip_port.h
//     CREATED: Sept 24, 2015
//      AUTHOR: Ananda Ravuri, Intel <ananda.ravuri@intel.com>
//              Joseph Grecco, Intel <joe.grecco@intel.com>
//
// PURPOSE:   This file contains the definations of the CCIP Port
//             Device Feature List and CSR.
// HISTORY:
// COMMENTS:
// WHEN:          WHO:     WHAT:
//****************************************************************************///
#ifndef __AALKERNEL_CCIP_PORT_DEF_H_
#define __AALKERNEL_CCIP_PORT_DEF_H_

#include <aalsdk/kernel/aaltypes.h>
#include "cci_pcie_driver_internal.h"
#include "ccip_defs.h"

BEGIN_NAMESPACE(AAL)


///============================================================================
/// Name: port_device
/// @brief  Port device struct
///============================================================================
struct port_device
{
   struct CCIP_PORT_HDR         *m_pport_hdr;         // PORRT Header
   struct CCIP_PORT_DFL_ERR     *m_pport_err;         // PORT Error DFL
   struct CCIP_PORT_DFL_UMSG    *m_pport_umsg;        // PORT USMG DFL
   struct CCIP_PORT_DFL_PR      *m_pport_pr;          // PORT PR DFL
   struct CCIP_PORT_DFL_STAP    *m_pport_stap;        // PORT Signal tap DFL

   struct ccip_device           *m_ccipdev;           // Parent board

   // Used for being added to the list of devices.
   kosal_list_head               m_list;

   // Private semaphore
   struct semaphore              m_sem;

   enum aal_bus_types_e          m_bustype;
   btUnsigned32bitInt            m_busNum;
   btUnsigned32bitInt            m_devicenum;   // device number
   btUnsigned32bitInt            m_functnum;    // function number

   // The AFU in this port
   struct afu_device             m_afu;


}; // end struct port_device

#define ccip_port_dev_pci_dev(pdev)          ((pdev)->m_pcidev)

#define afu_dev_to_cci_dev(ptr)              cci_container_of(ptr, struct afu_device, m_afu, struct port_device)

#define ccip_port_afu_dev(pdev)               ((pdev)->m_afu)
#define ccip_port_hdr(pdev)                   ((pdev)->m_pport_hdr)
#define ccip_port_err(pdev)                   ((pdev)->m_pport_err)
#define ccip_port_umsg(pdev)                  ((pdev)->m_pport_umsg)
#define ccip_port_pr(pdev)                    ((pdev)->m_pport_pr)
#define ccip_port_stap(pdev)                  ((pdev)->m_pport_stap)

#define ccip_port_bustype(pdev)               ((pdev)->m_bustype)
#define ccip_port_busnum(pdev)                ((pdev)->m_busNum)
#define ccip_port_devnum(pdev)                ((pdev)->m_devicenum)
#define ccip_port_fcnnum(pdev)                ((pdev)->m_functnum)

#define ccip_port_fcnnum(pdev)                ((pdev)->m_functnum)

#define ccip_port_mem_sessionp(pdev)          ((pdev)->m_pmem_session)

#define ccip_port_mmio(pdev)                  ((pdev)->m_phys_port_mmio)
#define ccip_port_kvp_mmio(pdev)              ((pdev)->m_kvp_port_mmio)
#define ccip__port_len_mmio(pdev)             ((pdev)->m_len_port_mmio)

#define ccip_port_to_ccidev(pdev)             ((pdev)->m_ccipdev)

#define ccip_port_list_head(pdev)             ((pdev)->m_list)
#define cci_list_to_cci_port_device(plist)     kosal_list_entry(plist, struct port_device, m_list)


#define aaldev_to_ccip_port_device(plist)         kosal_list_entry(plist, struct port_device, m_list)
#define ccip_port_to_PIPsessionp(pdev)        ((pdev)->m_pPIPSession)
#define ccip_port_psem(pdev)                  (&(pdev)->m_sem)

/// @brief creates a Port Device
///
/// @param[in] pkvp_port_mmio port mmio virtual address
/// @return    port_device code
struct port_device  *get_port_device( btVirtAddr pkvp_port_mmio);

/// @brief  destroys a Port Device
///
/// @param[in] pkvp_port_mmio port mmio virtual address
/// @return    port_device code
void destroy_port_device( struct port_device  *);


/// @brief   reads PORT Header
///
/// @param[in] pport_dev port device pointer.
/// @param[in] pkvp_port_mmio port mmio virtual address
/// @return    error code
struct CCIP_PORT_HDR * get_port_header( btVirtAddr );

/// @brief   reads PORT feature list
///
/// @param[in] pport_dev port device pointer.
/// @param[in] pkvp_port_mmio port mmio virtual address
/// @return    error code
bt32bitInt get_port_featurelist(struct port_device *,btVirtAddr );

btVirtAddr get_port_feature( struct port_device *,btUnsigned64bitInt );

extern struct aal_ipip cci_Portpip;
extern struct aal_ipip cci_AFUpip;

END_NAMESPACE(AAL)

#endif /* __AALKERNEL_CCIP_PORT_DEF_H_ */
