// Copyright (c) 2012-2015, Intel Corporation
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
/// @file AALServiceModule.cpp
/// @brief AALServiceModule implementation.  The AAL Service Module is an
///        object embedded in the Service Library that:
///        - Implements interface to outside for Service Construction
///        - Keeps track of all Services constructed through it.
///        - NOTE: Some Services may expose more objects than are tracked by
///                the ServiceModule. For example a singleton Service may
///                expose smart pointers or Proxies to allow the singleton to
///                to be shared. This type service may appear as 1 Service.
///                It is up to the Service to track these sub-objects.
/// @ingroup Services
/// @verbatim
/// Intel(R) QuickAssist Technology Accelerator Abstraction Layer
///
/// AUTHORS: Joseph Grecco, Intel Corporation
///          Tim Whisonant, Intel Corporation
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 01/22/2013     TSW      Moving C++ inlined definitions to .cpp file
/// 09/15/2015     JG       Redesigned to fix flow bugs@endverbatim
//****************************************************************************
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif // HAVE_CONFIG_H

#include "aalsdk/aas/AALServiceModule.h"
#include "aalsdk/osal/ThreadGroup.h"

#include "aalsdk/aas/AALService.h"
#include "aalsdk/Dispatchables.h"

BEGIN_NAMESPACE(AAL)

//=============================================================================
// Name: AALServiceModule()
// Description: Constructor
//=============================================================================
AALServiceModule::AALServiceModule(ISvcsFact &fact) :
   m_SvcsFact(fact),
   m_RuntimeClient(NULL),
   m_pendingcount(0)
{
   if ( SetSubClassInterface(iidServiceProvider, dynamic_cast<IServiceModule *>(this)) != EObjOK ) {
      m_bIsOK = false;
      return;
   }
}

//=============================================================================
// Name: ~AALServiceModule()
// Description: Destructor
//=============================================================================
AALServiceModule::~AALServiceModule()
{
}
btBool AALServiceModule::Construct(IRuntime           *pAALRuntime,
                                    IBase               *Client,
                                    TransactionID const &tranID,
                                    NamedValueSet const &optArgs)
{

   AutoLock(this);

   // Create the actual object
   IBase *pNewService = m_SvcsFact.CreateServiceObject( this,
                                                        pAALRuntime);
   // Add the service to the list of services the module
   if ( NULL == pNewService ) {
      return false;
   }

   // Keep track of outstanding transactions so that
   //  we don't disappear before they are complete.
   m_pendingcount++;

   // Initialize the Service. It  will issue serviceAllocated or failure.
   //   When the Service finishes initalization it will indicate in callback
   //   whether it was successful or not.
   if( !m_SvcsFact.InitializeService( pNewService,
                                      Client,
                                      tranID,
                                      optArgs)){
      // If InitializeService fails then this is a severe failure.
      // An event was not sent. Let upper layers handle the failure.
      m_SvcsFact.DestroyServiceObject(pNewService);
      return false;
   }
   return true;
}

//=============================================================================
// Name: Destroy()
// Description: Destroy all registered Services
// Interface: public
// Inputs: none
// Outputs: none.
// Comments:
//=============================================================================
void AALServiceModule::Destroy()
{
   // Protect the counter calculation
   //  so nothing can be released until
   //  we are waiting
   {
      AutoLock(this);

      list_type::size_type size = m_serviceList.size();
      if ( 0 == size ) {
         return;
      }

      // Initialize the semaphore as a count up by initializing
      //  count to a negative number.
      //  The waiter will block until the semaphore
      //  counts up to zero.
      btBool res = m_srvcCount.Create( - static_cast<btInt>(size) );
      ASSERT(res);

      // Loop through all services and shut them down
      SendReleaseToAll();
   }

   // Wait for all to complete. Unlock before waiting.
   m_srvcCount.Wait();
}

//=============================================================================
// Name: ServiceReleased()
// Description: Callback invoked when a Servcie has been released
// Interface: public
// Inputs: none
// Outputs: none.
// Comments:
//=============================================================================
void AALServiceModule::ServiceReleased(IBase *pService)
{
   AutoLock(this);
   RemovefromServiceList(pService);
}

//=============================================================================
// Name: ServiceInitialized()
// Description: Callback invoked when the Service has been successfully
//              initialized
// Interface: public
// Inputs: none
// Outputs: none.
// Comments:
//=============================================================================
btBool AALServiceModule::ServiceInitialized( IBase *pService,
                                             TransactionID const &rtid)
{
   AutoLock(this);

   // Reduce pending count
   m_pendingcount--;

   ASSERT(NULL != pService);
   IServiceBase *pServiceBase = dynamic_ptr<IServiceBase>(iidServiceBase, pService);

   ASSERT(NULL != pServiceBase);
   if(NULL == pServiceBase ){
      return false;
   }

   // Add Service to the List
   btBool ret = AddToServiceList(pService);
   ASSERT(true == ret);
   if(false == ret){
      return ret;
   }

   // Notify the Service client on behalf of the Service
   return pServiceBase->getRuntime()->schedDispatchable(new ServiceClientCallback( ServiceClientCallback::Allocated,
                                                                                   pServiceBase->getServiceClient(),
                                                                                   pServiceBase->getRuntimeClient(),
                                                                                   pService,
                                                                                   rtid));


}

//=============================================================================
// Name: ServiceInitFailed()
// Description: Callback invoked when the Service failed to initialize.
// Interface: public
// Inputs: none
// Outputs: none.
// Comments: Once the Service calls this it will be destroyed!! The Service
//           MUST return immediately with the return code.
//=============================================================================
btBool AALServiceModule::ServiceInitFailed(IBase *pService,
                                           IEvent const *pEvent)
{
   AutoLock(this);
   btBool ret = false;

   m_pendingcount--;

   ASSERT(NULL != pService);
   IServiceBase *pServiceBase = dynamic_ptr<IServiceBase>(iidServiceBase, pService);

   ASSERT(NULL != pServiceBase);
   if(NULL == pServiceBase ){
      return false;
   }

   // Create the dispachable for the Service allocate failed callback
   ServiceClientCallback * pDisp = new ServiceClientCallback( ServiceClientCallback::AllocateFailed,
                                                              pServiceBase->getServiceClient(),
                                                              pServiceBase->getRuntimeClient(),
                                                              pService,
                                                              pEvent);

   // Destroy the failed Service
   m_SvcsFact.DestroyServiceObject(pService);

   // Notify the Service client on behalf of the Service
   FireAndForget(pDisp);
   return ret;
}

//=============================================================================
// Name: AddToServiceList()
// Description: Add a Service to the list of constructed Services
// Interface: public
// Inputs: none
// Outputs: none.
// Comments:
//=============================================================================
btBool AALServiceModule::AddToServiceList(IBase *pService)
{
   AutoLock(this);

   if ( ServiceInstanceRegistered(pService) ) {
      return false;
   }

   m_serviceList.push_front(pService);

   return true;
}

//=============================================================================
// Name: RemovefromServiceList()
// Description: Remove a Released Service
// Interface: public
// Inputs: none
// Outputs: none.
// Comments:
//=============================================================================
btBool AALServiceModule::RemovefromServiceList(IBase *pService)
{
   AutoLock(this);

   if ( !ServiceInstanceRegistered(pService) ) {
      return false;
   }
   list_iter itr = find(m_serviceList.begin(), m_serviceList.end(), pService);
   m_serviceList.erase(itr);

   // Post to the count up semaphore
   //  in case the service is shutting down
   m_srvcCount.Post(1);

   return true;
}

//=============================================================================
// Name: ServiceInstanceRegistered()
// Description: Determine if the Service has already been registered
// Interface: public
// Inputs: none
// Outputs: none.
// Comments:
//=============================================================================
btBool AALServiceModule::ServiceInstanceRegistered(IBase *pService)
{
   AutoLock(this);
   return m_serviceList.end() != find(m_serviceList.begin(), m_serviceList.end(), pService);
}

//=============================================================================
// Name: SendReleaseToAll()
// Description: Broadcast a Release to all Services
// Interface: public
// Inputs: none
// Outputs: none.
// Comments: calls Release in reverse order
//=============================================================================
void AALServiceModule::SendReleaseToAll()
{
   AutoLock(this);   // Lock until done issuing releases
   CSemaphore     srvcCount;


   list_iter iter = m_serviceList.begin();

   btUnsigned32bitInt size = static_cast<btUnsigned32bitInt>(m_serviceList.size());
   if ( 0 == size ) {
      return;
   }

   //  count to a negative number.
   //  The waiter will block until the semaphore
   //  counts up to zero.
   if(!m_srvcCount.Reset( - static_cast<btInt>(size))){
      return;
   }

   while ( m_serviceList.end() != iter ) {

      // Get the IAALService from the IBase
      IAALService *pService = dynamic_ptr<IAALService>(iidService, *iter);

      iter++;

      if ( NULL != pService ) {
         // Release the Service overriding the default delivery
         pService->Release(TransactionID(dynamic_cast<IBase*>(this),true));
      }
   }
}

// <IServiceClient>
//=============================================================================
// Name: SendReleaseToAll()
// Description: Broadcast a hard Release to all Services
// Interface: public
// Inputs: none
// Outputs: none.
// Comments: THIS IS HARD CORE AND MAY WANT TO BE REMOVED
//=============================================================================
void AALServiceModule::serviceAllocated(IBase               *pServiceBase,
                                        TransactionID const &rTranID )
{

}

//=============================================================================
// Name: SendReleaseToAll()
// Description: Broadcast a hard Release to all Services
// Interface: public
// Inputs: none
// Outputs: none.
// Comments: THIS IS HARD CORE AND MAY WANT TO BE REMOVED
//=============================================================================
void AALServiceModule::serviceAllocateFailed(const IEvent &rEvent)
{

}

//=============================================================================
// Name: SendReleaseToAll()
// Description: Broadcast a hard Release to all Services
// Interface: public
// Inputs: none
// Outputs: none.
// Comments: THIS IS HARD CORE AND MAY WANT TO BE REMOVED
//=============================================================================
void AALServiceModule::serviceReleased(TransactionID const &rTranID )
{

   m_srvcCount.Post(1);
}


//=============================================================================
// Name: SendReleaseToAll()
// Description: Broadcast a hard Release to all Services
// Interface: public
// Inputs: none
// Outputs: none.
// Comments: THIS IS HARD CORE AND MAY WANT TO BE REMOVED
//=============================================================================
void AALServiceModule::serviceReleaseFailed(const IEvent &rEvent)
{

}

//=============================================================================
// Name: SendReleaseToAll()
// Description: Broadcast a hard Release to all Services
// Interface: public
// Inputs: none
// Outputs: none.
// Comments: THIS IS HARD CORE AND MAY WANT TO BE REMOVED
//=============================================================================
void AALServiceModule::serviceEvent(const IEvent &rEvent)
{

}


END_NAMESPACE(AAL)
