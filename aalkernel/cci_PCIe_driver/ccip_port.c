//******************************************************************************
// This  file  is  provided  under  a  dual BSD/GPLv2  license.  When using or
//         redistributing this file, you may do so under either license.
//
//                            GPL LICENSE SUMMARY
//
//  Copyright(c) 2015-2016, Intel Corporation.
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
//  Copyright(c) 2015-2016, Intel Corporation.
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
/// @file ccip_port.c
/// @brief  Implementation of the ccip Port object.
/// @ingroup aalkernel_ccip
/// @verbatim
//        FILE: ccip_port.c
//     CREATED: Sept 24, 2015
//      AUTHOR:
//
// PURPOSE:   This file contains the implementation of the CCIP port
//             low-level function (i.e., Physical Interface Protocol driver).
// HISTORY:
// COMMENTS:
// WHEN:          WHO:     WHAT:
//****************************************************************************///
#include "aalsdk/kernel/kosal.h"
#define MODULE_FLAGS CCIPCIE_DBG_MOD

#include "aalsdk/kernel/aalbus.h"
#include "aalsdk/kernel/AALTransactionID_s.h"
#include "aalsdk/kernel/aalbus-ipip.h"
#include "aalsdk/kernel/ccip_defs.h"

#include "aalsdk/kernel/ccipdriver.h"
#include "ccipdrv-events.h"

#include "ccip_port.h"
#include "cci_pcie_driver_PIPsession.h"

#define  CCIP_PORT_OUTSTADREQ_TIMEOUT   100
#define  CCIP_PORT_OUTSTADREQ_COMPLETE  0x1

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
////////////////////                                     //////////////////////
/////////////////           AAL SUPPORT FUNCTIONS          ////////////////////
////////////////////                                     //////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int CommandHandler( struct aaldev_ownerSession *,
                           struct aal_pipmessage*);

// TODO going to try and use a common mmapper for all obkects
extern int cci_mmap( struct aaldev_ownerSession *pownerSess,
                     struct aal_wsid *wsidp,
                     btAny os_specific );

//=============================================================================
// Name: cci_FMEpip
// @brief Physical Interface Protocol Interface for the CCIP Port device
//=============================================================================
struct aal_ipip cci_Portpip = {
   .m_messageHandler = {
      .sendMessage   = CommandHandler,       // Command Handler
      .bindSession   = BindSession,          // Session binder
      .unBindSession = UnbindSession,        // Session unbinder
   },

   .m_fops = {
     .mmap = cci_mmap,
   },

   // Methods for binding and unbinding PIP to generic aal_device
   //  Unused in this PIP
   .binddevice    = NULL,      // Binds the PIP to the device
   .unbinddevice  = NULL,      // Binds the PIP to the device
};


///============================================================================
/// Name: cci_create_AAL_Port_Device
/// @brief Creates and registers PR objects (resources) we want to
///        expose through AAL.
///
/// @param[in] pportdev - Port device
/// @param[in] devnum - Port number
/// @param[in] paalid - Base AAL ID for this device.
/// @return    AAL Device pointer
///============================================================================
struct cci_aal_device   *
               cci_create_AAL_Port_Device( struct port_device   *pportdev,
                                           btUnsigned16bitInt    devnum,
                                           struct aal_device_id *paalid)
{
   struct cci_aal_device   *pcci_aaldev = NULL;
   int                      ret         = 0;
   PTRACEIN;

   //=====================================================================
   // Create the Port AAL device. The Port AAL device is the class
   //   is registered with AAL Bus to enable it to be allocated by an
   //   application. AAL device objects represent the application usable
   //   devices. AAL device objects have their own low level communication
   //   function called the Physical Interface Protocol (PIP). This enables
   //   us to easily create object specific interfaces in a single driver.

   // Construct the cci_aal_device object
   pcci_aaldev = cci_create_aal_device();

   ASSERT(NULL != pcci_aaldev);
   if(NULL == pcci_aaldev ) {
      return NULL;
   }


   // Make it an Port by setting the type field and giving a pointer to the
   //  Port device object of the CCIP board device
   cci_aaldev_type(pcci_aaldev)  = cci_dev_Port;
   cci_aaldev_pport(pcci_aaldev) = pportdev;
   cci_aaldev_pfme(pcci_aaldev)  = ccip_port_dev_fme(pportdev);

   // Save the PCI device handle
   cci_aaldev_pci_dev(pcci_aaldev) = ccip_dev_to_pci_dev( ccip_port_to_ccidev(pportdev) );


   // Setup the AAL device's ID. This is the collection of attributes
   //  that uniquely identifies the AAL device, usually for the purpose
   //  of allocation through Resource Management
   //------------------------------------------------------------------
   aaldevid_devaddr_bustype(*paalid)     =   ccip_port_bustype(pportdev);

   // The AAL address maps to the PCIe address. The Subdevice number is
   //  vendor defined and in this case the FME object has the value CCIP_DEV_FME_SUBDEV
   aaldevid_devaddr_busnum(*paalid)          = ccip_port_busnum(pportdev);
   aaldevid_devaddr_devnum(*paalid)          = ccip_port_devnum(pportdev);
   aaldevid_devaddr_fcnnum(*paalid)          = ccip_port_fcnnum(pportdev);
   aaldevid_devaddr_subdevnum(*paalid)       = devnum;
   aaldevid_devaddr_instanceNum(*paalid)     = 0;
   aaldevid_devaddr_socketnum(*paalid)       = ccip_port_socketnum(pportdev);

   // The following attributes describe the interfaces supported by the device
   aaldevid_afuguidl(*paalid)            = CCIP_PORT_GUIDL;
   aaldevid_afuguidh(*paalid)            = CCIP_PORT_GUIDH;
   aaldevid_devtype(*paalid)             = aal_devtypeAFU;
   aaldevid_pipguid(*paalid)             = CCIP_PORT_PIPIID;
   aaldevid_vendorid(*paalid)            = AAL_vendINTC;

   cci_aaldev_phys_afu_mmio(pcci_aaldev)    = ccip_port_phys_mmio(pportdev);
   cci_aaldev_kvp_afu_mmio(pcci_aaldev)     = ccip_port_kvp_mmio(pportdev);
   cci_aaldev_len_afu_mmio(pcci_aaldev)     = ccip_port_mmio_len(pportdev);


   // Set the interface permissions
   // Enable MMIO-R
   cci_aaldev_set_allow_map_mmior_space(pcci_aaldev);


   // Create the AAL device and attach it to the CCI device object
   cci_aaldev_to_aaldev(pcci_aaldev)  =  aaldev_create( "CCIPPORT",           // AAL device base name
                                                        &*paalid,             // AAL ID
                                                        &cci_Portpip);
   //CCI device object create fails, delete Port AAL device
   if( NULL == cci_aaldev_to_aaldev(pcci_aaldev)) {
      cci_destroy_aal_device(pcci_aaldev);
      return NULL;
   }

   //===========================================================
   // Set up the optional aal_device attributes
   //

   // Set how many owners are allowed access to this device simultaneously
   cci_aaldev_to_aaldev(pcci_aaldev)->m_maxowners = 1;

   // Set the config space mapping permissions
   cci_aaldev_to_aaldev(pcci_aaldev)->m_mappableAPI = AAL_DEV_APIMAP_NONE;
   if( cci_aaldev_allow_map_csr_read_space(pcci_aaldev) ){
      cci_aaldev_to_aaldev(pcci_aaldev)->m_mappableAPI |= AAL_DEV_APIMAP_CSRWRITE;
   }

   if( cci_aaldev_allow_map_csr_write_space(pcci_aaldev) ){
      cci_aaldev_to_aaldev(pcci_aaldev)->m_mappableAPI |= AAL_DEV_APIMAP_CSRREAD;
   }

   if( cci_aaldev_allow_map_mmior_space(pcci_aaldev) ){
      cci_aaldev_to_aaldev(pcci_aaldev)->m_mappableAPI |= AAL_DEV_APIMAP_MMIOR;
   }

   if( cci_aaldev_allow_map_umsg_space(pcci_aaldev) ){
      cci_aaldev_to_aaldev(pcci_aaldev)->m_mappableAPI |= AAL_DEV_APIMAP_UMSG;
   }

   // The PIP uses the PIP context to get a handle to the CCI Device from the generic device.
   aaldev_pip_context(cci_aaldev_to_aaldev(pcci_aaldev)) = (void*)pcci_aaldev;

   // Method called when the device is released (i.e., its destructor)
   //  The canonical release device calls the user's release method.
   //  If NULL is provided then only the canonical behavior is done
   dev_setrelease(cci_aaldev_to_aaldev(pcci_aaldev), cci_release_device);

      // Device is ready for use.  Publish it with the Configuration Management Subsystem
   ret = cci_publish_aaldevice(pcci_aaldev);
   ASSERT(ret == 0);
   if(0> ret){
      PERR("Failed to initialize AAL Device for FME[%d:%d:%d:%x:%d]",aaldevid_devaddr_busnum(*paalid),
                                                                     aaldevid_devaddr_devnum(*paalid),
                                                                     aaldevid_devaddr_fcnnum(*paalid),
                                                                     aaldevid_devaddr_subdevnum(*paalid),
																     aaldevid_devaddr_instanceNum(*paalid));
      cci_destroy_aal_device(pcci_aaldev);
      return NULL;
   }
   PTRACEOUT;
   return pcci_aaldev;
}

//=============================================================================
// Name: CommandHandler
// Description: Implements the PIP command handler
// Interface: public
// Inputs: pownerSess - Session between App and device
//         Message - Message to process
// Outputs: none.
// Comments:
//=============================================================================
int
CommandHandler(struct aaldev_ownerSession *pownerSess,
               struct aal_pipmessage      *Message)
{
#if (1 == ENABLE_DEBUG)
#define AFU_COMMAND_CASE(x) case x : PDEBUG("%s\n", #x);
#else
#define AFU_COMMAND_CASE(x) case x :
#endif // ENABLE_DEBUG

   // Private session object set at session bind time (i.e., when object allocated)
   struct cci_PIPsession *pSess = (struct cci_PIPsession *)aalsess_pipHandle(pownerSess);
   struct cci_aal_device  *pdev  = NULL;

   // Overall return value for this function. Set before exiting if there is an error.
   //    retval = 0 means good return.
   int retval = 0;

   // if we return a request error, return this.  usually it's an invalid request error.
   uid_errnum_e request_error = uid_errnumInvalidRequest;

   // UI Driver message
   struct aalui_CCIdrvMessage *pmsg = (struct aalui_CCIdrvMessage *) Message->m_message;

   // Save original response buffer size in case we return something
   btWSSize         respBufSize     = Message->m_respbufSize;

   // Assume returning nothing. By setting the response buffer size to 0
   //   we tell the upstream side that there is no payload to copy back.
   //   Setting it here means we don't have to set it (or forget to) in each
   //   command.  We've recorded the payload buffer size above if we do need
   //   intend to send a payload.
   Message->m_respbufSize          = 0;

   PTRACEIN;

   // Perform some basic checks while assigning the pdev
   ASSERT(NULL != pSess );
   if ( NULL == pSess ) {
      PDEBUG("Error: No PIP session\n");
      return -EIO;
   }

   // Get the cci device
   pdev = cci_PIPsessionp_to_ccidev(pSess);
   if ( NULL == pdev ) {
      PDEBUG("Error: No device\n");
      return -EIO;
   }

   //=====================
   // Message processor
   //=====================
   switch ( pmsg->cmd ) {

      // Returns a workspace ID for the Config Space
      AFU_COMMAND_CASE(ccipdrv_getMMIORmap) {
         struct aal_wsid  *wsidp = NULL;
         struct ccidrvreq *preq = (struct ccidrvreq *)pmsg->payload;
         struct aalui_WSMEvent WSID;

         // clear WSID structure
         memset(&WSID, 0, sizeof(struct aalui_WSMEvent));

         if ( !cci_aaldev_allow_map_mmior_space(pdev) ) {
            PERR("Failed ccipdrv_getMMIOR map Permission\n");
            PERR("Direct API access not permitted on this device\n");
            Message->m_errcode = uid_errnumPermission;
            break;
         }

         //------------------------------------------------------------
         // Create the WSID object and add to the list for this session
         //------------------------------------------------------------
         if ( WSID_MAP_MMIOR != preq->ahmreq.u.wksp.m_wsid ) {
            PERR("Failed ccipdrv_getMMIOR map Parameter\n");

            PERR("Bad WSID on ccipdrv_getMMIORmap\n");
            Message->m_errcode = uid_errnumBadParameter;
            break;
         }

         wsidp = ccidrv_getwsid(pownerSess->m_device, preq->ahmreq.u.wksp.m_wsid);
         if ( NULL == wsidp ) {
            PERR("Could not allocate workspace\n");
            retval = -ENOMEM;
            /* generate a failure event back to the caller? */
            goto ERROR;
         }

         wsidp->m_type = WSM_TYPE_MMIO;
         PDEBUG("Getting CSR %s Aperature WSID %p using id %llx .\n",
                   ((WSID_CSRMAP_WRITEAREA == preq->ahmreq.u.wksp.m_wsid) ? "Write" : "Read"),
                   wsidp,
                   preq->ahmreq.u.wksp.m_wsid);

         PDEBUG("Apt = %" PRIxPHYS_ADDR " Len = %d.\n",cci_aaldev_phys_afu_mmio(pdev), (int)cci_aaldev_len_afu_mmio(pdev));

         // Set up the return payload
         WSID.evtID           = uid_wseventMMIOMap;
         WSID.wsParms.wsid    = pwsid_to_wsidHandle(wsidp);
         WSID.wsParms.physptr = cci_aaldev_phys_afu_mmio(pdev);
         WSID.wsParms.size    = cci_aaldev_len_afu_mmio(pdev);

         // Make this atomic. Check the original response buffer size for room
         if(respBufSize >= sizeof(struct aalui_WSMEvent)){
            *((struct aalui_WSMEvent*)Message->m_response) = WSID;
            Message->m_respbufSize = sizeof(struct aalui_WSMEvent);
         }
         PDEBUG("Buf size =  %u Returning WSID %llx\n",(unsigned int)Message->m_respbufSize, WSID.wsParms.wsid  );
         Message->m_errcode = uid_errnumOK;

         // Add the new wsid onto the session
         aalsess_add_ws(pownerSess, wsidp->m_list);

      } break;


      AFU_COMMAND_CASE(ccipdrv_getFeatureRegion) {
         struct aal_wsid  *wsidp          = NULL;
         struct ccidrvreq     *preq       = (struct ccidrvreq *)pmsg->payload;
         struct aalui_WSMEvent WSID;
         btWSID                featurenum = preq->ahmreq.u.wksp.m_wsid;
         btPhysAddr            pFeature   = 0;
         btVirtAddr            pvFeature  = 0;

         // clear WSID structure
         memset(&WSID, 0, sizeof(struct aalui_WSMEvent));

         PVERBOSE("Getting feature ID %u\n",(unsigned int)featurenum);

          if( true != get_port_feature(cci_aaldev_pport(pdev),featurenum, &pFeature, &pvFeature)){
            Message->m_errcode = uid_errnumBadParameter;
            PTRACEOUT;
            return 0;
         }

         wsidp = ccidrv_getwsid(pownerSess->m_device, preq->ahmreq.u.wksp.m_wsid);
         if ( NULL == wsidp ) {
            PERR("Could not allocate workspace ID\n");
            retval = -ENOMEM;
            /* generate a failure event back to the caller? */
            goto ERROR;
         }

         wsidp->m_type = WSM_TYPE_MMIO;

         WSID.evtID           = uid_wseventMMIOMap;
         WSID.wsParms.wsid    = pwsid_to_wsidHandle(wsidp);
         WSID.wsParms.physptr = cci_aaldev_phys_afu_mmio(pdev);
         WSID.wsParms.size    = cci_aaldev_len_afu_mmio(pdev);

         // Make this atomic. Check the orignal response buffer size for room
         if(respBufSize >= sizeof(struct aalui_WSMEvent)){
            *((struct aalui_WSMEvent*)Message->m_response) = WSID;
            Message->m_respbufSize = sizeof(struct aalui_WSMEvent);
         }
      } break;

      AFU_COMMAND_CASE(ccipdrv_getPortError) {

          bt32bitInt res               = 0;
          struct CCIP_ERROR port_error = {0};
          PVERBOSE("ccipdrv_getPortError \n");

          res= get_port_error(cci_aaldev_pport(pdev) ,&port_error);
          if(0 != res ) {
             Message->m_errcode = uid_errnumBadParameter;
             break;
          }

         if(respBufSize >= sizeof(struct CCIP_ERROR)){
             PVERBOSE("ccipdrv_getPortError  Valid Buffer\n");
            *((struct CCIP_ERROR*)Message->m_response) = port_error;
            Message->m_respbufSize = sizeof(struct CCIP_ERROR);
         }

         // Success Event
         Message->m_errcode = uid_errnumOK;

       } break; // case ccipdrv_getPortError

       AFU_COMMAND_CASE(ccipdrv_ClearAllPortErrors) {

          // Clear ALL Port errors
          // 1) Check for AP6 State
          // 2) Reset Port
          // 3) Set PORT mask to all 1's to mask errors
          // 4) Clear all errors
          // 5) Set PORT mask to all 0's to enable errors.
          // 6) All errors starts capturing new errors
          // 7) Enable port

          struct CCIP_PORT_STATUS local_port_status ;
          Get64CSR(&cci_aaldev_pport(pdev)->m_pport_hdr->ccip_port_status.csr,&local_port_status.csr);

          // if Device in AP6 state don't clear any PORT errors
          if( AFU_Power_AP6 == local_port_status.afu_pwr_state  ) {

             PERR("Could not clear all errors, device in AP6 state. \n");
             Message->m_errcode = uid_errnumAP6Detected;
             break;
          }

          // Reset PORT if AP6 event detected.
          if( 0x1 == ccip_port_err(cci_aaldev_pport(pdev))->ccip_port_error.ap6_event) {
             // Soft Reset PORT before Clearing AP6 Error
             if(0 != port_afu_quiesce_and_halt( cci_aaldev_pport(pdev) )){
                // Failure
                PERR(" Port Reset Timeout error. \n");
                Message->m_errcode = uid_errnumTimeout;
                break;
             }

          }

          //Set PORT mask to all 1's to mask errors
          ccip_port_err(cci_aaldev_pport(pdev))->ccip_port_error_mask.csr        = CLEAR_ALL_ERRORS;

          //Clear all errors
          ccip_port_err(cci_aaldev_pport(pdev))->ccip_port_error.csr             = CLEAR_ALL_ERRORS;
          ccip_port_err(cci_aaldev_pport(pdev))->ccip_port_first_error.csr       = CLEAR_ALL_ERRORS;

          //Set PORT mask to all 0's to enable errors.
          ccip_port_err(cci_aaldev_pport(pdev))->ccip_port_error_mask.csr        = 0x0;

          // Enable Port after clearing Errors.
          if(0 != port_afu_Enable( cci_aaldev_pport(pdev) )){
             PERR(" Port Enable timeout error. \n");
             // Failure event
             Message->m_errcode = uid_errnumTimeout;
             break;
          }

          // Success Event
          Message->m_errcode = uid_errnumOK;

        } break; // case ccipdrv_ClearAllPortErrors

       AFU_COMMAND_CASE(ccipdrv_ClearPortError) {

          // Clear Port errors
          // 1) Check for AP6 State
          // 2) Reset Port
          // 3) Clear errors
          // 4) Port errors starts capturing new errors
          // 5) Enable port

          struct CCIP_PORT_ERROR  local_port_error;
          struct CCIP_PORT_STATUS local_port_status ;
          struct ccidrvreq *preq = (struct ccidrvreq *)pmsg->payload;

          if(NULL == preq) {
             Message->m_errcode = uid_errnumBadParameter;
             break;
          }

          PVERBOSE("Port Error  CSR:%llX \n",ccip_port_err(cci_aaldev_pport(pdev))->ccip_port_error.csr);
          PVERBOSE("New Port Error  CSR:%llX \n",preq->ahmreq.u.error_csr.error0);

          // Port error to local CSR
          local_port_error.csr = preq->ahmreq.u.error_csr.error0 ;

          // Checking for AP6 Error
          if(0x1 == local_port_error.ap6_event) {

             Get64CSR(&cci_aaldev_pport(pdev)->m_pport_hdr->ccip_port_status.csr,&local_port_status.csr);

             if( AFU_Power_AP6 == local_port_status.afu_pwr_state  ) {
                PERR("Could not clear error, device in AP6 state. \n");
                Message->m_errcode = uid_errnumAP6Detected;
                break;
             }

             // Reset PORT if AP6 event detected.
             if( 0x1 == ccip_port_err(cci_aaldev_pport(pdev))->ccip_port_error.ap6_event) {
                // Soft Reset PORT before Clearing AP6 Error
                if(0 != port_afu_quiesce_and_halt( cci_aaldev_pport(pdev) )){
                   // Failure
                   PERR(" Port Reset Timeout error. \n");
                   Message->m_errcode = uid_errnumTimeout;
                   break;
                }

             }
          }

          //Clear  errors
          ccip_port_err(cci_aaldev_pport(pdev))->ccip_port_error.csr             = preq->ahmreq.u.error_csr.error0;
          ccip_port_err(cci_aaldev_pport(pdev))->ccip_port_first_error.csr       = preq->ahmreq.u.error_csr.error0;

          // Enable PORT after clearing AP6 Error
          if(0x1 == local_port_error.ap6_event) {

             // Enable Port after clearing Errors.
             if(0 != port_afu_Enable( cci_aaldev_pport(pdev) )){
                PERR(" Port Enable Timeout error. \n");
                // Failure event
                Message->m_errcode = uid_errnumTimeout;
                break;
             }

          }

          // Success Event
          Message->m_errcode = uid_errnumOK;

        } break; // case ccipdrv_ClearPortError


        AFU_COMMAND_CASE(ccipdrv_SetPortErrorMask) {

           struct ccidrvreq *preq = (struct ccidrvreq *)pmsg->payload;

           if(NULL == preq) {
              Message->m_errcode = uid_errnumBadParameter;
              break;
           }

           PVERBOSE("Port mask CSR:%llX \n",cci_aaldev_pport(pdev)->m_pport_err->ccip_port_error_mask.csr);
           PVERBOSE("New Port Mask CSR:%llX \n",preq->ahmreq.u.error_csr.error0);

           ccip_port_err(cci_aaldev_pport(pdev))->ccip_port_error_mask.csr =preq->ahmreq.u.error_csr.error0;

           // Success Event
           Message->m_errcode = uid_errnumOK;

         } break; // case ccipdrv_SetPortErrorMask

   default: {

      PDEBUG("Unrecognized command %" PRIu64 " or 0x%" PRIx64 " in AFUCommand\n", pmsg->cmd, pmsg->cmd);
      Message->m_errcode = request_error;
      retval = -EINVAL;

   } break;
   } // switch (pmsg->cmd)

   ASSERT(0 == retval);

ERROR:
   return retval;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
////////////////////                                     //////////////////////
/////////////////          GENERIC PORT FUNCTIONS          ////////////////////
////////////////////                                     //////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


///============================================================================
/// Name: get_port_device
/// @brief   Creates a Port device.
///
/// @param[in] pkvp_port_mmio - Port MMIO region
/// @return    error code
///============================================================================
struct port_device  *get_port_device( btPhysAddr pphys_port_mmio,
                                      btVirtAddr pkvp_port_mmio,
                                      btWSSize   port_mmio_len)
{
   struct port_device      *pportdev   = NULL;
   bt32bitInt               res         = 0;

   PTRACEIN;

   // Validate inputs parameters
   ASSERT(pkvp_port_mmio);
   if( NULL ==  pkvp_port_mmio ) {
      return NULL;
   }

   // Construct the new object
   pportdev =(struct port_device*) kosal_kzmalloc(sizeof(struct port_device));
   ASSERT(pportdev);
   if(NULL == pportdev){
      PERR("Error allocating Port device\n");
      return NULL;
   }

   // Initialize the list head
   kosal_list_init(&ccip_port_list_head(pportdev));

   // Initialize uAFU pointer
   ccip_port_uafu_dev(pportdev) = NULL;

   ccip_port_kvp_mmio(pportdev)    = pkvp_port_mmio;
   ccip_port_phys_mmio(pportdev)   = pphys_port_mmio;
   ccip_port_mmio_len(pportdev)    = port_mmio_len;

   // Get Port header
   ccip_port_hdr(pportdev) = get_port_header(pkvp_port_mmio );
   if(NULL == ccip_port_hdr(pportdev)) {
      PERR("Error reading Port Header\n");
      goto ERR;
   }

   // get port feature list
   res =  get_port_featurelist(pportdev,pkvp_port_mmio );
   if(res !=0) {
      PERR("Port device feature list Error %d \n",res);
      goto ERR;
   }

   PINFO(" get_port_mmio EXIT \n");
   return pportdev;

ERR:
   kosal_kfree(pportdev, sizeof(struct port_device));

   PERR("Error getting Port device\n");
   PTRACEOUT;
   return NULL;
}

///============================================================================
/// Name: destroy_port_device
/// @brief   Destroys a Port device.
///
/// @param[in] pkvp_port_mmio - Port MMIO region
/// @return    error code
///============================================================================
void destroy_port_device( struct port_device  * pport_dev)
{
   kosal_kfree(pport_dev, sizeof(struct port_device));
   return;
}

///============================================================================
/// Name: get_port_header
/// @brief   reads PORT Header
///
/// @param[in] pport_dev port device pointer.
/// @param[in] pkvp_port_mmio port mmio virtual address
/// @return    Pointer to port header
///============================================================================
struct CCIP_PORT_HDR *get_port_header( btVirtAddr pkvp_port_mmio )
{
   return (struct CCIP_PORT_HDR *)(pkvp_port_mmio);
}

///============================================================================
/// Name: get_port_feature
/// @brief   Gets the pointer to a Port Feature
///
/// @param[in] pport_dev port device pointer.
/// @param[in] Feature_ID - Feature ID to search for
/// @return    NULL = failure
///============================================================================
btBool get_port_feature( struct port_device *pport_dev,
                             btUnsigned64bitInt Feature_ID,
                             btPhysAddr *pPhysAddr,
                             btVirtAddr *pVirtAddr)
{
   struct CCIP_DFH         port_dfh;
   btVirtAddr              pkvp_port   = NULL;
   btPhysAddr              pphys_port  = 0;

   PTRACEIN;

   if( ccip_port_hdr(pport_dev)->ccip_port_dfh.next_DFH_offset ==0)   {
      PERR("NO PORT features are available \n");
      return false;
   }
   // read PORT Device feature Header
   pkvp_port = ((btVirtAddr)ccip_port_hdr(pport_dev)) + ccip_port_hdr(pport_dev)->ccip_port_dfh.next_DFH_offset;
   port_dfh.csr = ccip_port_hdr(pport_dev)->ccip_port_dfh.csr;

   // Track the physical address as we walk the list
   pphys_port = ccip_port_phys_mmio(pport_dev) + ccip_port_hdr(pport_dev)->ccip_port_dfh.next_DFH_offset;

   do {
      // sanity check
      if (pkvp_port >= ((btVirtAddr)ccip_port_hdr(pport_dev)) + pport_dev->m_port_mmio_len) {
         PERR("Next DFH offset points beyond BAR!\n");
         PERR("   next_DFH_offset: 0x%x\n", port_dfh.next_DFH_offset);
         PERR("   DFH            : 0x%0llx\n", port_dfh.csr);
         return false;
      }
      // Peek at the Header
      port_dfh.csr = read_ccip_csr64(pkvp_port,0);

     // Device feature ID
      if(Feature_ID == port_dfh.Feature_ID){
         PVERBOSE("Feature found.\n");
         if(NULL!= pPhysAddr){
            *pPhysAddr = pphys_port;
         }
         if(NULL!= pVirtAddr){
            *pVirtAddr = pkvp_port;
         }
         PTRACEOUT;

         return true;
      }

      // Point at next feature header.
      pkvp_port = pkvp_port + port_dfh.next_DFH_offset;
      pphys_port = pphys_port + port_dfh.next_DFH_offset;

   }while(0 != port_dfh.next_DFH_offset ); // end while

   PVERBOSE("Feature not found\n");
   PTRACEOUT;
   return false;
}

///============================================================================
/// Name: get_port_featurelist
/// @brief   reads PORT feature list
///
/// @param[in] pport_dev port device pointer.
/// @param[in] pkvp_port_mmio port mmio virtual address
/// @return    error code
///============================================================================
bt32bitInt get_port_featurelist( struct port_device *pport_dev,
                                 btVirtAddr pkvp_port_mmio )
{

   bt32bitInt              res =0;
   struct CCIP_DFH         port_dfh;
   btVirtAddr              pkvp_port = NULL;


   PTRACEIN;
   PINFO(" get_port_featurelist ENTER\n");

   if( ccip_port_hdr(pport_dev)->ccip_port_dfh.next_DFH_offset ==0)   {
      PERR("NO PORT features are available \n");
      res = -EINVAL;
      goto ERR;
   }
   // read PORT Device feature Header
   pkvp_port = pkvp_port_mmio + ccip_port_hdr(pport_dev)->ccip_port_dfh.next_DFH_offset;

   do {
      // Peek at the Header
      port_dfh.csr = read_ccip_csr64(pkvp_port,0);

      // check for Device type
      // Type == CCIP_DFType_private
      if(port_dfh.Type  != CCIP_DFType_private ) {
         PERR(" invalid PORT Feature Type \n");
         res = -EINVAL;
         goto ERR;

      }

      if(0 != port_dfh.Feature_rev){
         PVERBOSE("Found Port feature with invalid revision %d. IGNORING!\n",port_dfh.Feature_rev);
         continue;
      }

      // Device feature ID
      switch(port_dfh.Feature_ID)
      {

         case CCIP_PORT_DFLID_ERROR:
         {
            ccip_port_err(pport_dev) = (struct CCIP_PORT_DFL_ERR *)pkvp_port;
         }
         break;

         case CCIP_PORT_DFLID_USMG:
         {
            ccip_port_umsg(pport_dev) = (struct CCIP_PORT_DFL_UMSG *)pkvp_port;
         }
         break;

         case CCIP_PORT_DFLID_PR:
         {
            ccip_port_pr(pport_dev) = (struct CCIP_PORT_DFL_PR *)pkvp_port;
         }
         break;

         case CCIP_PORT_DFLID_STP:
         {
            ccip_port_stap(pport_dev) = (struct CCIP_PORT_DFL_STAP *)pkvp_port;
         }
         break;

         default :
         {
            PERR(" invalid PORT Feature ID\n");
            res = -EINVAL;
            goto ERR;
         }
         break ;
      } // end switch

      // Point at next feature header.
      pkvp_port = pkvp_port + port_dfh.next_DFH_offset;

   }while(0 != port_dfh.next_DFH_offset ); // end while

ERR:
   PINFO(" get_port_featurelist EXIT \n");
   PTRACEOUT_INT(res);
   return res;
}

///============================================================================
/// Name: port_afu_Enable
/// @brief   Port Enable
///
/// @param[in] pport_dev port device pointer
/// @return    error code
///============================================================================
bt32bitInt port_afu_Enable(struct port_device *pport_dev)
{

   bt32bitInt res = 0;

   PTRACEIN;

   if (NULL == ccip_port_hdr(pport_dev))
   {
      res = -EINVAL;
      return  res;
   }
   ccip_port_hdr(pport_dev)->ccip_port_control.port_sftreset_control = 0x0;

   PTRACEOUT_INT(res);

   return res;
}
///============================================================================
/// Name: port_afu_reset
/// @brief   Reset port
///
/// @param[in] pport_dev port device pointer
/// @return    error code
///============================================================================
bt32bitInt port_afu_reset(struct port_device *pport_dev)
{
   bt32bitInt res = 0;

   PTRACEIN;

   if (NULL == ccip_port_hdr(pport_dev))  {
      res = -EINVAL;
      return  res;
   }

   // afu/port Quiesce reset
   res = port_afu_quiesce_and_halt(pport_dev);
   if (0 != res) {
      goto ERR;
   }

   // afu/port enable
   res = port_afu_Enable(pport_dev);
   if (0 != res) {
      goto ERR;
   }

ERR:
   PTRACEOUT_INT(res);
   return res;
}

///============================================================================
/// Name: port_afu_quiesce_and_halt
/// @brief   Port Quiesce Reset
///
/// @param[in] pport_dev port device pointer
/// @return    0 = success
///============================================================================
bt32bitInt port_afu_quiesce_and_halt(struct port_device *pport_dev)
{
   bt32bitInt res = 0;
   btTime delay = 10;
   btTime totaldelay = 0;

   PTRACEIN;

   if (NULL == ccip_port_hdr(pport_dev))  {
      res = 1;
      goto ERR;
   }

   // Reset Port
   ccip_port_hdr(pport_dev)->ccip_port_control.port_sftreset_control = 0x1;

   // All CCI-P request at port complete
   // Set to 1 When all outstanding requests initiated by this port have been drained
   while (CCIP_PORT_OUTSTADREQ_COMPLETE != ccip_port_hdr(pport_dev)->ccip_port_control.ccip_outstanding_request)
   {
      // Sleep
      kosal_udelay(delay);

      // total delay
      totaldelay = totaldelay + delay;

      // if total delay is more then 1 millisecond , return error
      if (totaldelay > CCIP_PORT_OUTSTADREQ_TIMEOUT)   {
         PERR("Timed out tryin to quiesce AFU");
         res = 1;
         goto ERR;
      }

   }

ERR:
   PTRACEOUT_INT(res);
   return  res;
}


///============================================================================
/// Name:    get_port_error
/// @brief   get fpga global error
///
/// @param[in] pport_dev port device pointer.
/// @param[in] pport_error ccip error structure  pointer
/// @return    error code
///============================================================================
bt32bitInt get_port_error(struct port_device* pport_dev,
                          struct CCIP_ERROR*  pport_error)
{
   int res = 0;

   PTRACEIN;

   if( (NULL == pport_dev) || (NULL == pport_error))  {
         PERR("Invalid input pointers \n");
         res =-EINVAL;
         goto ERR;
      }

   memset(pport_error,0,sizeof(struct CCIP_ERROR));

   pport_error->error0_mask    = ccip_port_err(pport_dev)->ccip_port_error_mask.csr;
   pport_error->error0         = ccip_port_err(pport_dev)->ccip_port_error.csr;
   pport_error->first_error    = ccip_port_err(pport_dev)->ccip_port_first_error.csr;

   pport_error->malreq0        = ccip_port_err(pport_dev)->ccip_port_malformed_req_0.csr;
   pport_error->malreq1        = ccip_port_err(pport_dev)->ccip_port_malformed_req_1.csr;

   PTRACEOUT_INT(res);
   return res;

ERR:
   PTRACEOUT_INT(res);
   return  res;

}


