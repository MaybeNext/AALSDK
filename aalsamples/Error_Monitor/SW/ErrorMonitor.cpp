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
/// @file ErrorMonitor.cpp
/// @brief Basic AFU interaction.
/// @ingroup ErrorMonitor
/// @verbatim
/// Accelerator Abstraction Layer Sample Application
///
///    This application is for example purposes only.
///    It is not intended to represent a model for developing commercially-deployable applications.
///    It is designed to show working examples of the AAL programming model and APIs.
///
/// AUTHORS: Ananda Ravuri, Intel Corporation.
///
/// This Sample demonstrates the following:
///    - The basic structure of an AAL program using the AAL APIs.
///    - The IHelloAAL and IHelloAALClient interfaces of HelloAALService.
///    - System initialization and shutdown.
///    - Use of interface IDs (iids).
///    - Creating a AFU class that exposes a proprietary interface.
///    - Invoking a method on an AFU using a proprietary interface.
///    - Accessing object interfaces through the Interface functions.
///
/// This sample is designed to be used with SampleAFU1.
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 03/09/2016     SC       Initial version started based on older sample code.@endverbatim
//****************************************************************************
#include <aalsdk/AAL.h>
#include <aalsdk/Runtime.h>
#include <aalsdk/AALLoggerExtern.h> // Logger
#include <signal.h>
#include <aalsdk/service/IALIAFU.h>

using namespace std;
using namespace AAL;

// Convenience macros for printing messages and errors.

#ifdef MSG
# undef MSG
#endif // MSG
#if 0
   #define MSG(x) std::cout << __AAL_SHORT_FILE__ << ':' << __LINE__ << ':' << __AAL_FUNC__ << "() : " << x << std::endl
#else
   #define MSG(x)
#endif
#ifdef ERR
# undef ERR
#endif // ERR
#define ERR(x) std::cerr << __AAL_SHORT_FILE__ << ':' << __LINE__ << ':' << __AAL_FUNC__ << "() **Error : " << x << std::endl

// Print/don't print the event ID's entered in the event handlers.
#if 1
# define EVENT_CASE(x) case x : MSG(#x);
#else
# define EVENT_CASE(x) case x :
#endif

// FME Error CSR Offset
#define FME_ERR_MASK      0x4008
#define FME_ERR           0x4010
#define FME_FIRST_ERR     0x4018

// PORT Error CSR Offset
#define PORT_ERR_MASK     0x1008
#define PORT_ERR          0x1010
#define PORT_FIRST_ERR    0x1018
#define PORT_MALFORM0     0x1020
#define PORT_MALFORM1     0x1028

#define FOUND_ERR  1

// FME GUID
#define CCIP_FME_AFUID              "BFAF2AE9-4A52-46E3-82FE-38F0F9E17764"
// PORT GUID
#define CCIP_PORT_AFUID             "3AB49893-138D-42EB-9642-B06C6B355B87"


// doxygen HACK to generate correct class diagrams
#define RuntimeClient ErrorMonRuntimeClient
/// @addtogroup HelloAAL
/// @{

/// @brief   Define our Service client class so that we can receive Service-related notifications from the AAL Runtime.
///          The Service Client contains the application logic.
///
/// When we request an AFU (Service) from AAL, the request will be fulfilled by calling into this interface.
class ErrorMonApp: public CAASBase, public IRuntimeClient, public IServiceClient
{
public:
   ErrorMonApp();
   ~ErrorMonApp();
   /// @brief Called by the main part of the application,Returns 0 if Success
   ///
   /// Application Requests Service using Runtime Client passing a pointer to self.
   /// Blocks calling thread from [Main} untill application is done.
   btInt run(btBool bClear); //Return 0 if success

   // Gets FME Errors
   btInt getFMEError();
   // Gets Port errors
   btInt getPortError();

   // Gets FME and PORT Error CSR masks
   void getFMEPortErrorMask();

   // Clears FME and Port Errors
   void clearErrors();
   // prints Port Errors
   void printPortError(btUnsigned64bitInt port_error_csr);

   btBool isOK()  {return m_bIsOK;}

   // <begin IServiceClient interface>
   void serviceAllocated(IBase *pServiceBase,
                         TransactionID const &rTranID);

   void serviceAllocateFailed(const IEvent &rEvent);

   void serviceReleaseFailed(const IEvent &rEvent);

   void serviceReleased(TransactionID const &rTranID);

   void serviceEvent(const IEvent &rEvent);
   // <end IServiceClient interface>

   // <begin IRuntimeClient interface>
    void runtimeCreateOrGetProxyFailed(IEvent const &rEvent);

    void runtimeStarted(IRuntime            *pRuntime,
                        const NamedValueSet &rConfigParms);

    void runtimeStopped(IRuntime *pRuntime);

    void runtimeStartFailed(const IEvent &rEvent);

    void runtimeStopFailed(const IEvent &rEvent);

    void runtimeAllocateServiceFailed( IEvent const &rEvent);

    void runtimeAllocateServiceSucceeded(IBase               *pClient,
                                         TransactionID const &rTranID);

    void runtimeEvent(const IEvent &rEvent);
  // <end IRuntimeClient interface>

protected:
    enum {
       FME,
       PORT
    };
   IBase            *m_pFMEService;     // The generic AAL Service interface for the FME.
   IBase            *m_pPortService ;   // The generic AAL Service interface for the Port.
   Runtime           m_Runtime;
   IALIMMIO         *m_pFMEMMIOService;
   IALIMMIO         *m_pPortMMIOService;
   CSemaphore        m_Sem;            // For synchronizing with the AAL runtime.
   btInt             m_Result;         // Returned result value; 0 if success
};

///////////////////////////////////////////////////////////////////////////////
///
///  MyServiceClient Implementation
///
///////////////////////////////////////////////////////////////////////////////
ErrorMonApp::ErrorMonApp() :
   m_pFMEService(NULL),
   m_pPortService(NULL),
   m_Runtime(this),
   m_pFMEMMIOService(NULL),
   m_pPortMMIOService(NULL),
   m_Result(0)
{
    // Publish our interfaces
    SetInterface(iidRuntimeClient, dynamic_cast<IRuntimeClient *>(this));
    SetInterface(iidServiceClient, dynamic_cast<IServiceClient *>(this));

    m_Sem.Create(0, 1);

    NamedValueSet configArgs;
    NamedValueSet configRecord;

    configRecord.Add(AALRUNTIME_CONFIG_BROKER_SERVICE, "librrmbroker");
    configArgs.Add(AALRUNTIME_CONFIG_RECORD, &configRecord);

    if(!m_Runtime.start(configArgs)){
       m_bIsOK = false;
       return;
    }

    m_Sem.Wait();
    m_bIsOK = true;
}

ErrorMonApp::~ErrorMonApp()
{
   m_Runtime.stop();
   m_Sem.Destroy();
}

void ErrorMonApp::clearErrors()
{
   btUnsigned64bitInt csr = 0xFFFFFFFFFFFFFFFF;

   // Clear FME Errors CSR
   m_pFMEMMIOService->mmioWrite64(FME_ERR, csr);
   m_pFMEMMIOService->mmioWrite64(FME_FIRST_ERR, csr);

   // Clear Port Errors CSR
   m_pPortMMIOService->mmioWrite64(PORT_ERR, csr);
   m_pPortMMIOService->mmioWrite64(PORT_FIRST_ERR, csr);
}

void ErrorMonApp::getFMEPortErrorMask()
{
   btUnsigned64bitInt mask_csr = 0x0;

   // Read FME Error Mask CSR
   m_pPortMMIOService->mmioRead64(FME_ERR_MASK, &mask_csr);
   if(0x0 != mask_csr) {
      cout << endl<<"FME mask CSR:0x"<< hex << mask_csr << endl;
   }

   // Read PORT Error Mask CSR
   m_pPortMMIOService->mmioRead64(PORT_ERR_MASK, &mask_csr);
   if(0x0 != mask_csr) {
      cout << endl<<"Port mask CSR:0x"<< hex << mask_csr << endl;
   }

}

btInt ErrorMonApp::getFMEError()
{
   btUnsigned64bitInt fme_csr = 0x0;
   int res                    = 0;

   // Read FME Error CSR
   m_pFMEMMIOService->mmioRead64(FME_ERR, &fme_csr);

   if(0x0 == fme_csr) {
      cout << endl << "No FME Errors Found " << endl;
   } else  {
      cout  << endl <<"==== FME ERRORS ====" <<endl;
      cout << "FME Error CSR:0x"<< hex << fme_csr << endl;
      res =FOUND_ERR;
   }

   // Read FME First Error CSR
   fme_csr=0x0;
   m_pFMEMMIOService->mmioRead64(FME_FIRST_ERR, &fme_csr);

   if(0x0 == fme_csr) {
      cout << endl << "No FME First Errors Found "<< endl;
   } else  {
      cout << endl <<"==== FME FIST ERRORS ====" << endl;
      cout << "FME First Error CSR:0x"<< hex << fme_csr << endl;
      res =FOUND_ERR;
   }
   return res ;
}

btInt ErrorMonApp::getPortError()
{
   btUnsigned64bitInt port_error_csr = 0x0;
   int res                     = 0;

   // Read PORT  Error CSR
   m_pPortMMIOService->mmioRead64(PORT_ERR, &port_error_csr);

   if(0x0 == port_error_csr) {
      cout << endl<<"No Port Errors Found "<< endl;
   } else {
      cout << "==== PORT ERRORS ===="<< endl;
      cout << "Port Error CSR:0x"<< hex << port_error_csr << endl;
      printPortError(port_error_csr);
      res =FOUND_ERR;
   }

   // Read PORT Fist Error CSR
   port_error_csr=0x0;
   m_pPortMMIOService->mmioRead64(PORT_FIRST_ERR, &port_error_csr);
   if(0x0 == port_error_csr) {
      cout << endl << "No Port First Errors Found "<< endl;
   } else {
      cout  << endl <<"==== PORT FIRST  ERRORS ====" << endl;
      cout << "Port First Error CSR:0x"<< hex << port_error_csr << endl;
      printPortError(port_error_csr);
      res =FOUND_ERR;
   }

   // Print both MALFORMED REQ0 or REQ1 CSRs if any PORT_ERROR detects
   port_error_csr=0x0;
   m_pPortMMIOService->mmioRead64(PORT_ERR, &port_error_csr);
   if(0x0 != port_error_csr) {

      // Read Port malformed Request0 Error CSR
      port_error_csr=0x0;
      m_pPortMMIOService->mmioRead64(PORT_MALFORM0, &port_error_csr);

      if(0x0 != port_error_csr) {
         cout <<  endl<<"==== PORT MALFORMED REQ 0 ===="<< endl;
         cout << "Port malformed request0 Error CSR:0x"<< hex << port_error_csr << endl;
         res =FOUND_ERR;
      }

      // Read Port malformed Request1 Error CSR
      port_error_csr =0x0;
      m_pPortMMIOService->mmioRead64(PORT_MALFORM1, &port_error_csr);
      if(0x0 != port_error_csr) {
         cout << endl <<"==== PORT MALFORMED REQ 1 ===="<< endl;
         cout << "Port malformed request1 Error CSR:0x"<< hex << port_error_csr << endl;
         res =FOUND_ERR;
      }

   } // end of if
   return res ;
}

void ErrorMonApp::printPortError(btUnsigned64bitInt port_error_csr)
{

   struct CCIP_PORT_ERROR {
      union {
         btUnsigned64bitInt csr;
         struct {
            btUnsigned64bitInt tx_ch0_overflow :1;       //Tx Channel0 : Overflow
            btUnsigned64bitInt tx_ch0_invalidreq :1;     //Tx Channel0 : Invalid request encoding
            btUnsigned64bitInt tx_ch0_req_cl_len3 :1;    //Tx Channel0 : Request with cl_len=3
            btUnsigned64bitInt tx_ch0_req_cl_len2 :1;    //Tx Channel0 : Request with cl_len=2
            btUnsigned64bitInt tx_ch0_req_cl_len4 :1;    //Tx Channel0 : Request with cl_len=4

            btUnsigned64bitInt rsvd :11;

            btUnsigned64bitInt tx_ch1_overflow :1;       //Tx Channel1 : Overflow
            btUnsigned64bitInt tx_ch1_invalidreq :1;     //Tx Channel1 : Invalid request encoding
            btUnsigned64bitInt tx_ch1_req_cl_len3 :1;    //Tx Channel1 : Request with cl_len=3
            btUnsigned64bitInt tx_ch1_req_cl_len2 :1;    //Tx Channel1 : Request with cl_len=2
            btUnsigned64bitInt tx_ch1_req_cl_len4 :1;    //Tx Channel1 : Request with cl_len=4


            btUnsigned64bitInt tx_ch1_insuff_datapayload :1; //Tx Channel1 : Insufficient data payload
            btUnsigned64bitInt tx_ch1_datapayload_overrun:1; //Tx Channel1 : Data payload overrun
            btUnsigned64bitInt tx_ch1_incorr_addr  :1;       //Tx Channel1 : Incorrect address
            btUnsigned64bitInt tx_ch1_sop_detcted  :1;       //Tx Channel1 : NON-Zero SOP Detected
            btUnsigned64bitInt tx_ch1_atomic_req  :1;        //Tx Channel1 : Illegal VC_SEL, atomic request VLO
            btUnsigned64bitInt rsvd1 :6;

            btUnsigned64bitInt mmioread_timeout :1;         // MMIO Read Timeout in AFU
            btUnsigned64bitInt tx_ch2_fifo_overflow :1;     //Tx Channel2 : FIFO overflow

            btUnsigned64bitInt rsvd2 :6;

            btUnsigned64bitInt num_pending_req_overflow :1; //Number of pending Requests: counter overflow

            btUnsigned64bitInt rsvd3 :23;
         }; // end struct
      }; // end union

   }ccip_port_error; // end struct CCIP_PORT_ERROR

   ccip_port_error.csr=port_error_csr;

   if(ccip_port_error.tx_ch0_overflow) {
      cout << "Tx Channel0 Overflow Set "<< endl;
   }
   if(ccip_port_error.tx_ch0_invalidreq) {
      cout << "Tx Channel0 Invalid request encoding Set " << endl;
   }
   if(ccip_port_error.tx_ch0_req_cl_len3) {
      cout << "Tx Channel0 Request with cl_len3 Set "<< endl;
   }
   if(ccip_port_error.tx_ch0_req_cl_len2) {
      cout << "Tx Channel0 Request with cl_len2 Set "<< endl;
   }
   if(ccip_port_error.tx_ch0_req_cl_len4) {
      cout << "Tx Channel0 Request with cl_len4 Set "<< endl;
   }
   if(ccip_port_error.tx_ch1_overflow) {
      cout << "Tx Channel1 Overflow Set "<< endl;
   }
   if(ccip_port_error.tx_ch1_invalidreq) {
      cout << "Tx Channel1 Invalid request encoding Set " << endl;
   }
   if(ccip_port_error.tx_ch1_req_cl_len3) {
      cout << "Tx Channel1 Request with cl_len3 Set "<< endl;
   }
   if(ccip_port_error.tx_ch1_req_cl_len2) {
      cout << "Tx Channel1 Request with cl_len2 Set " << endl;
   }
   if(ccip_port_error.tx_ch1_req_cl_len4) {
      cout << "Tx Channel1 Request with cl_len4 Set "<< endl;
   }
   if(ccip_port_error.tx_ch1_insuff_datapayload) {
      cout << "Tx Channel1 Insufficient data payload Set "<< endl;
   }
   if(ccip_port_error.tx_ch1_datapayload_overrun) {
      cout << "Tx Channel1 Data payload overrun Set " << endl;
   }
   if(ccip_port_error.tx_ch1_incorr_addr) {
      cout << "Tx Channel1 Incorrect address Set "<< endl;
   }
   if(ccip_port_error.tx_ch1_sop_detcted) {
      cout << "Tx Channel1 NON-Zero SOP Detected Set " << endl;
   }
   if(ccip_port_error.tx_ch1_atomic_req) {
      cout << "Tx Channel1 Atomic request VLO Set " << endl;
   }
   if(ccip_port_error.mmioread_timeout) {
      cout << "MMIO Read Timed out in AFU Set " << endl;
   }
   if(ccip_port_error.tx_ch2_fifo_overflow) {
      cout << "Tx Channel2 : FIFO overflow Set " << endl;
   }
   if(ccip_port_error.num_pending_req_overflow) {
      cout << "Number of pending Requests counter overflow Set " << endl;
   }

   if(ccip_port_error.rsvd) {
      cout << "Reserved bit 15:5 Set " << endl;
   }
   if(ccip_port_error.rsvd1) {
      cout << "Reserved bit 31:26 Set " << endl;
   }
   if(ccip_port_error.rsvd2) {
      cout << "Reserved bit 39:34 Set " << endl;
   }
   if(ccip_port_error.rsvd3) {
      cout << "Reserved bit 41:63 Set " << endl;
   }

}


btInt ErrorMonApp::run(btBool bClear)
{

   cout <<"===================================="<<endl;
   cout <<"= Error Monitor Sample ="<<endl;
   cout <<"===================================="<<endl;

   // Request our AFU.

   NamedValueSet Manifest;
   NamedValueSet ConfigRecord;
   btInt         res = 0;

   ConfigRecord.Add(AAL_FACTORY_CREATE_CONFIGRECORD_FULL_SERVICE_NAME, "libHWALIAFU");
   ConfigRecord.Add(AAL_FACTORY_CREATE_CONFIGRECORD_FULL_AIA_NAME, "libaia");
   ConfigRecord.Add(keyRegAFU_ID,CCIP_FME_AFUID);

   Manifest.Add(AAL_FACTORY_CREATE_CONFIGRECORD_INCLUDED, &ConfigRecord);
   Manifest.Add(AAL_FACTORY_CREATE_SERVICENAME, "Error Monitor");

   MSG("Allocating Service");


   // Allocate the Service and wait for it to complete by sitting on the
   // semaphore. The serviceAllocated() callback will be called if successful.
   // If allocation fails the serviceAllocateFailed() should set m_bIsOK appropriately.
   // (Refer to the serviceAllocated() callback to see how the Service's interfaces
   // are collected.)
   {
      // Allocate the FME Resource
      TransactionID afu_tid(ErrorMonApp::FME);
      m_Runtime.allocService(dynamic_cast<IBase *>(this), Manifest, afu_tid);
      m_Sem.Wait();
      if(!m_bIsOK){
         ERR("Allocation failed\n");
         goto done_0;
      }
   }

   // Modify the manifest for the PORT
    Manifest.Delete(AAL_FACTORY_CREATE_CONFIGRECORD_INCLUDED);
    ConfigRecord.Delete(keyRegAFU_ID);

    ConfigRecord.Add(keyRegAFU_ID,CCIP_PORT_AFUID);
    Manifest.Add(AAL_FACTORY_CREATE_CONFIGRECORD_INCLUDED, &ConfigRecord);

   {
      // Allocate the PORT Resource
      TransactionID fme_tid(ErrorMonApp::PORT);
      m_Runtime.allocService(dynamic_cast<IBase *>(this), Manifest, fme_tid);
      m_Sem.Wait();
      if(!m_bIsOK){
         ERR("Allocation failed\n");
         goto done_0;
      }
   }

   // clears FME and Port Errors
   if(bClear) {
      clearErrors();
   }

   getFMEPortErrorMask();

   // Prints FME Errors
   res = getPortError();
   if(0 != res)
       ++m_Result;   // record error

   // Prints PORT Errors
   res = getFMEError();
   if(0 != res)
      ++m_Result;   // record error

   // Clean-up and return
   // Release() the Service through the Services IAALService::Release() method

   // Release FME Resource
   (dynamic_ptr<IAALService>(iidService, m_pFMEService))->Release(TransactionID());
   m_Sem.Wait();

   // Release PORT Resource
   (dynamic_ptr<IAALService>(iidService, m_pPortService))->Release(TransactionID());
   m_Sem.Wait();

done_0:
   m_Runtime.stop();
   m_Sem.Wait();

   return m_Result;
}

// We must implement the IServiceClient interface (IServiceClient.h):

// <begin IServiceClient interface>
void ErrorMonApp::serviceAllocated(IBase *pServiceBase,
                                   TransactionID const &rTranID)
{

   if(rTranID.ID() == ErrorMonApp::FME) {
      // FME Resource  Allocation
      m_pFMEService = pServiceBase;
      ASSERT(NULL != m_pFMEService);
      if ( NULL == m_pFMEService ) {
         m_bIsOK = false;
         return;
      }

      m_pFMEMMIOService = dynamic_ptr<IALIMMIO>(iidALI_MMIO_Service, pServiceBase);
      ASSERT(NULL != m_pFMEMMIOService);
      if ( NULL == m_pFMEMMIOService ) {
         m_bIsOK = false;
         return;
      }

   } else if(rTranID.ID() == ErrorMonApp::PORT)  {
      //PORT Resource  Allocation
      m_pPortService = pServiceBase;
      ASSERT(NULL != m_pPortService);
      if ( NULL == m_pPortService ) {
         m_bIsOK = false;
         return;
      }

      m_pPortMMIOService = dynamic_ptr<IALIMMIO>(iidALI_MMIO_Service, pServiceBase);
      ASSERT(NULL != m_pPortMMIOService);
      if ( NULL == m_pPortMMIOService ) {
         m_bIsOK = false;
         return;
      }

   } else {
      // Wrong Transaction ID
      ERR("Failed to allocate Service");
      m_bIsOK = false;
      return;
   }

   m_Sem.Post(1);
}

void ErrorMonApp::serviceAllocateFailed(const IEvent &rEvent)
{
   IExceptionTransactionEvent * pExEvent = dynamic_ptr<IExceptionTransactionEvent>(iidExTranEvent, rEvent);
   ERR("Failed to allocate a Service");
   ERR(pExEvent->Description());
   m_bIsOK = false;
   m_Sem.Post(1);
}

 void ErrorMonApp::serviceReleaseFailed(const IEvent        &rEvent)
{
    MSG("Failed to Release a Service");
    m_bIsOK = false;
    m_Sem.Post(1);
 }

 void ErrorMonApp::serviceReleased(TransactionID const &rTranID)
 {
    MSG("Service Released");
    m_Sem.Post(1);
}

void ErrorMonApp::serviceEvent(const IEvent &rEvent)
{
   ERR("unexpected event 0x" << hex << rEvent.SubClassID());
}
// <end IServiceClient interface>

void ErrorMonApp::runtimeCreateOrGetProxyFailed(IEvent const &rEvent)
{
   MSG("Runtime Create or Get Proxy failed");
   m_bIsOK = false;
   m_Sem.Post(1);
}

void ErrorMonApp::runtimeStarted( IRuntime *pRuntime,
                                const NamedValueSet &rConfigParms)
{
   m_bIsOK = true;
   m_Sem.Post(1);
}

void ErrorMonApp::runtimeStopped(IRuntime *pRuntime)
{
   MSG("Runtime stopped");
   m_bIsOK = false;
   m_Sem.Post(1);
}

void ErrorMonApp::runtimeStartFailed(const IEvent &rEvent)
{
   IExceptionTransactionEvent * pExEvent = dynamic_ptr<IExceptionTransactionEvent>(iidExTranEvent, rEvent);
   ERR("Runtime start failed");
   ERR(pExEvent->Description());
   m_Sem.Post(1);
}

void ErrorMonApp::runtimeStopFailed(const IEvent &rEvent)
{
    MSG("Runtime stop failed");
    m_Sem.Post(1);
}

void ErrorMonApp::runtimeAllocateServiceFailed( IEvent const &rEvent)
{
   IExceptionTransactionEvent * pExEvent = dynamic_ptr<IExceptionTransactionEvent>(iidExTranEvent, rEvent);
   ERR("Runtime AllocateService failed");
   ERR(pExEvent->Description());
}

void ErrorMonApp::runtimeAllocateServiceSucceeded(IBase *pClient,
                                                    TransactionID const &rTranID)
{
   TransactionID const * foo = &rTranID;
   MSG("Runtime Allocate Service Succeeded");
}

void ErrorMonApp::runtimeEvent(const IEvent &rEvent)
{
   MSG("Generic message handler (runtime)");
}


void int_handler(int sig)
{
   cerr<< "SIGINT: stopping the server\n";
}

/// @}
//=============================================================================
// Name: main
// Description: Entry point to the application
// Inputs: none
// Outputs: none
// Comments: Main initializes the system. The rest of the example is implemented
//           in the objects.
//=============================================================================
int main(int argc, char *argv[])
{

   ErrorMonApp         theApp;
   int result = 0;
   btBool bClear = false;

   if( argc>1 ) { // process command line arg of "-c"
      if (0 == strcmp (argv[1], "-c")) bClear = true;
   }

   if(theApp.IsOK()){
      result = theApp.run( bClear );
   }else{
      MSG("App failed to initialize");
   }

   MSG("Done");
   return result;
}

