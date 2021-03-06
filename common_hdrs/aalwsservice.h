//******************************************************************************
// This  file  is  provided  under  a  dual BSD/GPLv2  license.  When using or
//         redistributing this file, you may do so under either license.
//
//                            GPL LICENSE SUMMARY
//
//  Copyright(c) 2008-2016, Intel Corporation.
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
//  Copyright(c) 2008-2016, Intel Corporation.
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
//  FILE: aalwsservice.h
//  Author:  Alvin Chen, Intel Corporation
//  Created: 08/12/2008
//
//  Description:
//      Accelerator Abstraction Layer (AAL)
//      Kernel Workspace Manager Service Module
//
// WHEN:          WHO:     WHAT:
// 08/12/2008     AC       Initial version started
// 11/11/2008     JG       Added legal header
// 11/25/2008     HM       Large merge
// 12/16/2008     JG       Began support for abort and shutdown
//                            Added Support for WSID object
//                            Major interface changes.
// 01/04/2009     HM       Updated Copyright
// 12/27/2009     JG       Added CSR WS type
// 11/12/2010     AG       check for NULL pws in AAL_CHECK_WORKSPACE macro
// 02/26/2013     AG       Add wsid tracking and validation routines
// 03/12/2013     JG       Added Windows MMAP wsids
//****************************************************************************
#ifndef __AALSDK_KERNEL_AALWSSERVICE_H__
#define __AALSDK_KERNEL_AALWSSERVICE_H__
#include <aalsdk/kernel/kosal.h>

BEGIN_NAMESPACE(AAL)

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
// API IIDs TODO should come from aal ids
#define AAL_WSAPI_IID_01         (0x21)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define AAL_CL_SIZE                     64
#define AAL_PGTABLE_CL_SIZE             0x80

/////////////////////////////////////////////////////////////////////////////////////
// *   Local Definitions
/////////////////////////////////////////////////////////////////////////////////////
#define AAL_WKSP_MAX_SUPERPAGES_NUM_BITS    (10)
#define AAL_WKSP_MAX_SUPERPAGES_NUM         (1 << AAL_WKSP_MAX_SUPERPAGES_NUM_BITS)    /* 1K, Defined in HW SPEC. */
#define AAL_WKSP_MAX_SUPERPAGE_SIZE_BITS    (23)
#define AAL_WKSP_MAX_SUPERPAGE_SIZE         (1 << AAL_WKSP_MAX_SUPERPAGE_SIZE_BITS )   /* 8M, Defined in HW SPEC. */
#define AAL_WKSP_MAX_SIZE                   ((btUnsigned64bitInt)AAL_WKSP_MAX_SUPERPAGE_SIZE * (btUnsigned64bitInt)AAL_WKSP_MAX_SUPERPAGES_NUM) /* we support 8G per WS */

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define AAL_CHECK_WORKSPACE( pws )          ( NULL != pws && (btUnsigned32bitInt)pws->m_superpage[0] == pws->m_id )



//=============================================================================
//=============================================================================
//                                WORKSPACE
//=============================================================================
//=============================================================================

//=============================================================================
// Name: aal_wsid
// Description: Wrapper object for the workspace ID
//=============================================================================
enum wstype
{
   WSM_TYPE_VIRTUAL,
   WSM_TYPE_PHYSICAL,
   WSM_TYPE_CSR,
   WSM_TYPE_MMIO
};
struct aal_wsid
{
   struct aal_device *m_device;     // Device
   btWSID             m_id;         // ID, and pointer to workspace structure
   btHANDLE           m_dmahandle;  // Optional DMA Handle
   btWSID             m_handle;     // Handle passed up to user mode
   kosal_map_handle   m_maphandle;  // Used by OS User mode mapping
   enum wstype        m_type;       // Type of allocation
   btWSSize           m_size;       // Size of workspace
   kosal_list_head    m_list;       // Device owner list it is on
   /* chain of allocated workspace IDs; head is in ui_driver */
   kosal_list_head    m_alloc_list;
};


#ifdef __i386__
#define wsidobjp_to_wid(id)    ((unsigned long long)id)
#define wsid_to_wsidobjp(id)   ((struct aal_wsid *) ( id ) )

#define pgoff_to_wsidobj(off)  ((struct aal_wsid *)  (( (unsigned long )(off)) << PAGE_SHIFT))

#else

#     define wsid_to_maphandle(pwsid)     ( pwsid->m_maphandle )

#  if   defined ( __AAL_WINDOWS__ )
// Do nothing for Windows
#     define wsid_to_wsidHandle(id) ( (btWSID)(id) )
#     define pwsid_to_wsidHandle( pwsid ) ( (btWSID)( pwsid->m_handle ) )

#     define wsid_to_wsidobjp(id)      ( (struct aal_wsid *)id )

#     define pgoff_to_wsidobj(off)     ( (struct aal_wsid *)(off))

#  elif defined ( __AAL_LINUX__ )


#     define wsid_to_wsidHandle(wsid)  ((btWSID)( (wsid) <<21 ))
//#     define wsidHandle_to_wsid(h)     ( ((btWSID)(off)) >>21 )

//#     define pgoff_to_wsid(off)        ( ((btWSID)(off)) >>19 )
#     define pgoff_to_wsidHandle(off)  ((btWSID)off<<12)

#     define pwsid_to_wsidHandle(pwsid) ((btWSID)(pwsid->m_handle))


// DEPRECATING
#     define pgoff_to_wsidobj(off)  ( (struct aal_wsid *)( (off) | 0xfff0000000000000ULL) )
#     define wsidobjp_to_wid(id)    ( (btWSID)(id) << PAGE_SHIFT )
#     define wsid_to_wsidobjp(id)   ( (struct aal_wsid *)( ( (btWSID)(id) >> PAGE_SHIFT ) | 0xfff0000000000000ULL ) )


#  endif
#endif

struct aaldev_ownerSession; //forward reference



//=============================================================================
// Name: aalwsservice
// Description: Service interface for the AAL workspace manager service. This
//              interface defines the methods used by clients of the workspace
//              service.
//=============================================================================
struct aal_wsservice
{
    ///////////////////////////////////////////////////////////////////////////////////
    // Allocate the workspace with fixed size
    // Output: the workspace ID
    int (*allocate)(    const struct aal_wsservice * const pthis,
                        const btWSSize size,
                        btWSID *const wsid /* out: wsid */
                        );
    int (*vallocate)(   const struct aal_wsservice * const pthis,
                        const btWSSize                     size,
                        btWSID * const                     wsid,   /* out: wsid */
                        const btWSSize                     pgsize  /* in: page size */
                        );

    ///////////////////////////////////////////////////////////////////////////////////
    // Free the workspace
    int (*free) (struct aal_wsservice *pthis, btWSID wsid);
    int (*vfree)(struct aal_wsservice *pthis, btWSID wsid);


#if defined( __AAL_LINUX__ )
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Map the workspace into user space
    int (*mmap)( struct aal_wsservice *pthis, btWSID wsid, struct vm_area_struct *pvam);
#endif // OS

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Get the page table base address
    void * (*get_pgtable)(  const struct aal_wsservice *pthis,
                            const btWSID                wsid,
                            btWSSize * const            pgtblsize,
                            btWSSize * const            pgsizeorder );

    void * (*get_workspace)(    const struct aal_wsservice   *pthis,
                                const btWSID                  wsid,
                                btWSSize *const               size );

    void * (*validate_pointer)( const struct aal_wsservice   *pthis,
                                const btWSID                  wsid,
                                const btVirtAddr              uvptr,
                                btWSSize *const               size );
};


END_NAMESPACE(AAL)

#endif // __AALSDK_KERNEL_AALWSSERVICE_H__

