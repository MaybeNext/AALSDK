// Copyright(c) 2015-2016, Intel Corporation
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
/// @file HelloAALService-internal.h
/// @brief Definitions for Hello AAL Service.
/// @ingroup hello_service
/// @verbatim
/// Accelerator Abstraction Layer Sample Application
///
///    This application is for example purposes only.
///    It is not intended to represent a model for developing commercially-deployable applications.
///    It is designed to show working examples of the AAL programming model and APIs.
///
/// AUTHOR: Joseph Grecco, Intel Corporation
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 04/12/2015     JG       Initial version@endverbatim
//****************************************************************************
#ifndef __HELLOAALSERVICE_INT_H__
#define __HELLOAALSERVICE_INT_H__
#include "HelloAALService.h" // Public AFU device interface
#include <aalsdk/aas/AALService.h>
#include <aalsdk/IServiceClient.h>
#include <aalsdk/osal/IDispatchable.h>

using namespace AAL;

/// @addtogroup hello_service
/// @{

//=============================================================================
// Name: HelloAALService
// Description: Simple  AAL Service
// Interface: IHelloAALService
// Comments:
//=============================================================================
/// @brief Simple AAL Service
class HelloAALService : public ServiceBase, public IHelloAALService
{
public:

   /// Macro defines the constructor for a loadable AAL service.
   ///  The first argument is your class name, the second argument is the
   ///  name of the Service base class this service is derived from. In this
   ///  example we use ServiceBase as it is the class that provides the
   ///  support for Software-only devices.  Hardware-supported services might
   ///  use DeviceServiceBase instead.
   ///
   /// Note that initializers can be declared here but are preceded by a comma
   ///  rather than a colon.
   ///
   /// The design pattern is that the constructor does minimal work. Here we are
   ///  registering the interfaces the service implements. The default (Subclass)
   ///  interface is ISampleAFUPing.  ServiceBase provides an init() method that
   ///  can be used where more sophisticated initialization is required. The
   ///  init() method is called by the factory AFTER construction but before use.
   DECLARE_AAL_SERVICE_CONSTRUCTOR(HelloAALService, ServiceBase),
      m_pSvcClient(NULL),
      m_pClient(NULL)
   {
      SetInterface(iidSampleHelloAAL, dynamic_cast<IHelloAALService *>(this));
   }

   /// @brief Hook to allow the object to initialize.
   ///
   /// This function is called by the factory AFTER construction and AFTER
   /// _init(), insuring that base class initialization has occurred by the
   /// time this is called.
   /// @param[in] pclientBase A pointer to the Service Client interface.
   /// @param[in] optArgs A reference to the optional arguments passed to allocService().
   /// @param[in] rtid A reference to the Transaction ID.
   /// @retval True if the initialization succeeds.
   /// @retval False if a severe failure prevented sending a response or calling
   ///               initFailed().
   btBool init( IBase *pclientBase,
                NamedValueSet const &optArgs,
                TransactionID const &rtid);

   /// Send a message to the AFU.
   /// @param[in] sMessage The message to send.
   /// @param[in] rTranID A reference to the Transaction ID.
   /// @return void
   void Hello( btcString            sMessage,
               TransactionID const &rTranID);

   // Called when the service is released
   /// Release the Service and its resources.
   /// @param[in] rTranID A reference to the Transaction ID.
   /// @param[in] timeout The maximum time to wait for the Service to be released.
   /// @return void
   btBool Release(TransactionID const &rTranID, btTime timeout=AAL_INFINITE_WAIT);

protected:
   IServiceClient        *m_pSvcClient;
   IHelloAALClient       *m_pClient;
   TransactionID          m_CurrTranID;
};

/// @brief AAL - aware event for generating the response.
class HelloAppDispatchable : public IDispatchable
{
public:
   /// Dispatchable class for sending the response to the client.
   /// @param[in] pSvcClient A pointer to the HelloAAL Client.
   /// @param[in] pService A pointer to the HelloAAL Service.
   /// @param[in] rTranID A reference to the Transaction ID.
   /// @return void
   HelloAppDispatchable(IHelloAALClient *pSvcClient, IBase *pService, TransactionID const &rTranID);

   //  operator()
   /// Dispatch Event.
   ///
   /// Functors can be created by overriding this function.
   ///
   /// <B>Parameters</B> [in]  target Assumed to be of type EventHandler.
   /// @return void
   virtual void operator() ();

protected:
   IHelloAALClient      *m_pSvcClient;
   IBase                *m_pSevice;
   TransactionID const  &m_TranID;
};

/// @}

#endif //__SAMPLEAFU1SERVICE_INT_H__

