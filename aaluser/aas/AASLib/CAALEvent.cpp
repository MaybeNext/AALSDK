// Copyright (c) 2006-2015, Intel Corporation
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
/// @file CAALEvent.cpp
/// @brief CAALEvent, CTransactionEvent, CExceptionEvent, and CExceptionTransactionEvent
///  implementations.
/// @ingroup Events
/// @verbatim
/// Intel(R) QuickAssist Technology Accelerator Abstraction Layer
///
/// AUTHORS: Joseph Grecco, Intel Corporation
///          Henry Mitchel, Intel Corporation
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 01/04/2006     JG       Initial version started
/// 02/14/2007     JG       Refactored event base class to
///                         remove implementation inheritance
///                         of I_LRObjectInterface.
///                         Changed constant definitions and
///                         exports to export base pointer rather
///                         than object allowing the object
///                         subclass to remain opaque. User uses
///                         I_LREvent * and defines in
///                         libLongRidgeCPP.h
/// 03/21/2007     JG       Ported to linux
/// 04/24/2007     JG       Made all events carry IBase pointers
/// 05/28/2007     JG       Added cached interfaces (SubClass)
/// 06/08/2007     JG       Converted to btID keys
/// 10/04/2007     JG       Renamed to CAALEvent.cpp
/// 12/16/2007     JG       Changed include path to expose aas/
/// 02/03/2008     HM       Cleaned up various things brought out by ICC
/// 05/08/2008     HM       Comments & License
/// 12/10/2008     HM       Changed IEvent.Context(), no longer caches
/// 12/15/2008     HM       Added iidEvent to second constructor
/// 01/04/2009     HM       Updated Copyright
/// 02/25/2009     HM       Re-enabled IEvent.Context cacheing - required e.g.
///                            for DestroyObject messages@endverbatim
//****************************************************************************
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif // HAVE_CONFIG_H

#include "aalsdk/CAALEvent.h"
#include "aalsdk/INTCDefs.h"
#include "aalsdk/AALLogger.h"

#ifdef __ICC                           /* Deal with Intel compiler-specific overly sensitive remarks */
//   #pragma warning( push)
//   #pragma warning(disable:68)         // warning: integer conversion resulted in a change of sign.
                                       //    This is intentional
   #pragma warning(disable:383)        // remark: value copied to temporary, reference to temporary used
//   #pragma warning(disable:981)        // remark: operands are evaluated in unspecified order
//   #pragma warning(disable:869)        // remark: parameter "XXXX" was never referenced
//   #pragma warning(disable:1418)       // remark: external function definition with no prior declaration
//   #pragma warning(disable:1572)       // remark: floating-point equality and inequality comparisons are unreliable
#endif

BEGIN_NAMESPACE(AAL)

IEvent::~IEvent() {}
ITransactionEvent::~ITransactionEvent() {}
IExceptionEvent::~IExceptionEvent() {}
IApplicationEvent::~IApplicationEvent() {}
IApplicationExceptionEvent::~IApplicationExceptionEvent() {}


//=============================================================================
// Name: CAALEvent
// Description: Constructor
// Comments: Concrete Base class for all events
//=============================================================================
CAALEvent::CAALEvent(IBase *pObject, IMessageHandler *pHandler) :
   CriticalSection(),
   m_pObject(NULL),
   m_bIsOK(false),
   m_Context(NULL),
   m_pMessageHandler(pHandler),
   m_InterfaceMap(),
   m_ISubClass(),
   m_SubClassID(0)
{
   AutoLock(this);

   if ( SetInterface(iidCEvent, dynamic_cast<CAALEvent *>(this)) != EObjOK ) {
      return;
   }

   // IEvent is the default native subclass interface unless overriden by a subclass
   if ( SetSubClassInterface(iidEvent, dynamic_cast<IEvent *>(this)) != EObjOK ) {
      return;
   }

   // Save the objects
   m_pObject = pObject;

   // Event inherits object's Application Context Value based on m_pObject
   UpdateContext();

   m_bIsOK = true;
}


//=============================================================================
// Name: CAALEvent
// Description: Constructor
// Comments: Concrete Base class for all events
//=============================================================================
CAALEvent::CAALEvent(IBase *pObject, btIID SubClassID, IMessageHandler *pHandler) :
   CriticalSection(),
   m_pObject(NULL),
   m_bIsOK(false),
   m_Context(NULL),
   m_pMessageHandler(pHandler),
   m_InterfaceMap(),
   m_ISubClass(),
   m_SubClassID(0)
{
   AutoLock(this);

   if ( SetInterface(iidCEvent, dynamic_cast<CAALEvent *>(this)) != EObjOK ) {
      return;
   }

   if ( SetInterface(iidEvent, dynamic_cast<IEvent *>(this)) != EObjOK ) {
      return;
   }

   // IEvent is the default native subclass interface unless overriden by a subclass
   if ( SetSubClassInterface(SubClassID, dynamic_cast<IEvent *>(this)) != EObjOK ) {
      return;
   }

   // Save the object
   m_pObject = pObject;

   // Event inherits object's Application Context Value based on m_pObject
   UpdateContext();

   m_bIsOK = true;
}

//=============================================================================
// Name: CAALEvent
// Description: Copy constructor Constructor
// Comments:
//=============================================================================
CAALEvent::CAALEvent(const CAALEvent &rOther) :
   CriticalSection(),
   m_pObject(NULL),
   m_bIsOK(false),
   m_Context(NULL),
   m_pMessageHandler(NULL),
   m_InterfaceMap(),
   m_ISubClass(),
   m_SubClassID(0)
{
   AutoLock(this);

   //Self register the interface
   if ( SetInterface(iidCEvent, dynamic_cast<CAALEvent *>(this)) != EObjOK ) {
      return;
   }

   // IEvent is the default native subclass interface unless overriden by a subclass
   if ( SetSubClassInterface(iidEvent, dynamic_cast<IEvent *>(this)) != EObjOK ) {
      return;
   }

   m_pObject = rOther.m_pObject;
   m_Context = rOther.m_Context;
   m_pMessageHandler = rOther.m_pMessageHandler;
   m_bIsOK   = rOther.m_bIsOK;
}


//=============================================================================
// Name: CAALEvent::Interface
// Description: Gets a pointer to the requested interface
// Interface: public
// Inputs: Interface - name of the interface to get.
// Outputs: Interface pointer.
// Comments:
//=============================================================================
btGenericInterface CAALEvent::Interface(btIID Interface) const {
   AutoLock(this);

   if ( !Has(Interface) ) {
      return NULL;
   }

   IIDINTERFACE_CITR itr = m_InterfaceMap.find(Interface);
   if ( m_InterfaceMap.end() == itr ) { // not found
      return NULL;
   }

   return (*itr).second;
}

//=============================================================================
// Name: CAALEvent::SetSubClassInterface
// Description: Sets an interface pointer on the subclass interface for the
//              object.  This function may only be called once per class.
// Interface: protected
// Inputs: Interface - name of the interface to set.
//         pInterface - Interface pointer
// Outputs: Interface pointer.
// Comments:
//=============================================================================
EOBJECT CAALEvent::SetSubClassInterface(btIID              InterfaceID,
                                        btGenericInterface pInterface) {
   EOBJECT result;
   if( (result = SetInterface(InterfaceID,
                              pInterface)) != EObjOK ) {
      return result;
   }
   m_ISubClass  = pInterface;
   m_SubClassID = InterfaceID;
   return result;
}

//=============================================================================
// Name: CAALEvent::ISubClass
// Description: Returns the cached pointer to the native (subclass) object
// Interface: public
// Inputs: none.
// Outputs: class pointer
// Comments:
//=============================================================================
btGenericInterface CAALEvent::ISubClass() const
{
   return m_ISubClass;
}

//=============================================================================
// Name: CAALEvent::SubClassID
// Description: Returns the subclass ID for the Object
// Interface: public
// Inputs: none
// Outputs: ID
// Comments:
//=============================================================================
btIID CAALEvent::SubClassID() const
{
   return m_SubClassID;
}

//=============================================================================
// Name: CAALEvent::Has
// Description: Returns whether an object has an interface.
// Interface: public
// Inputs: Interface - name of the interface.
// Outputs: true - has interface otherwise false
// Comments:
//=============================================================================
btBool CAALEvent::Has(btIID Interface) const
{
   AutoLock(this);
   IIDINTERFACE_CITR itr = m_InterfaceMap.end();

   // Find the named value pair
   if ( m_InterfaceMap.find(Interface) == itr ) {
      return false;
   }
   return true;
}

//=============================================================================
// Name: CAALEvent::operator !=
// Description: operator !=
// Interface: public
// Inputs: rOther - other object to compare against.
// Outputs: != ? true : flase.
// Comments: equality is defined as implementing the same
//           interfaces NOT the contents of the objects data.
//=============================================================================
btBool CAALEvent::operator != (IEvent &rOther)
{
   AutoLock(this);

   CAALEvent *pOther = (CAALEvent*)rOther.Interface(iidCEvent);
   if ( pOther != NULL ) {
      return (m_InterfaceMap != pOther->m_InterfaceMap);
   }
   return true;
}

//=============================================================================
// Name: CAALEvent::operator ==
// Description: operator ==
// Interface: public
// Inputs: rOther - other object to compare against.
// Outputs: == ? true : flase.
// Comments: equality is defined as implementing the same
//           interfaces NOT the contents of the objects data.
//=============================================================================
btBool CAALEvent::operator == (IEvent &rOther)
{
   AutoLock(this);

   CAALEvent *pOther = (CAALEvent *)rOther.Interface(iidCEvent);
   if ( pOther != NULL ) {
      return (m_InterfaceMap == pOther->m_InterfaceMap);
   }
   return false;
}

//=============================================================================
// Name: CAALEvent::operator ()
// Description: Funtor operator
// Interface: public
// Inputs: context - payload
// Comments: Canonical processing:
//                 By default Events can only be delivered to static Event
//                 Handlers (btEventhandler) pre 4.0 or to IMessageHanders
//                 TransactionID() processing for event delivery override can
//                 ONLY be sent to btEventhandler types. Override this operator
//                 to alter behavior.
//=============================================================================
void CAALEvent::operator()()
{
   if ( ProcessEventTranID() ) {
      return;
   }

   if ( NULL != m_pMessageHandler ) {
      m_pMessageHandler->messageHandler(*this);
      delete this;
   } else if ( NULL != m_pEventHandler ) {
      m_pEventHandler(*this);
   }
}

//=============================================================================
// Name:         CAALEvent::ProcessEventTranID
// Description:  Process the transaction ID for Event Handling
// Interface:    protected
// Comments:
//=============================================================================
btBool CAALEvent::ProcessEventTranID()
{
   btBool ret = false;

   // If a static handler has been assigned
   if ( NULL != m_TranID.Handler() ) {
      //Call it
      m_TranID.Handler()(*this);
      ret =  m_TranID.Filter();
      // If an IMessageHandler has been assigned
   }
   if (  NULL != m_TranID.MsgHandler() ) {
      // Call it
      m_TranID.MsgHandler()->messageHandler(*this);
      ret = m_TranID.Filter();
   }
   // Return filter value or false if no override at all
   return ret;
}

//=============================================================================
// Name:          CAALEvent::SetObject
// Description:   Update the object pointed to by the Event, and its associated
//                   cached context
// Interface:     public
// Inputs:        pointer to new IBase
// Outputs:       void
// Comments:      Used by ReThrow, where the message needs to be modified but
//                   not completely reconstructed
//=============================================================================
void CAALEvent::SetObject(IBase *pObject)
{
   m_pObject = pObject;
   UpdateContext();
}

//=============================================================================
// Name:          CAALEvent::Dispatch
// Description:   Dispatch self to proper recipient
// Interface:     public
// Inputs:        Target object
// Outputs:       void
// Comments:      Default is AAL Event handler
//=============================================================================
void CAALEvent::Dispatch(btObjectType target) const
{
   btEventHandler eventHandler = reinterpret_cast<btEventHandler>(target);

   ASSERT(NULL != eventHandler);
   if ( NULL != eventHandler ) { // BREAKPOINT Event Dispatch
      eventHandler(*this);
   }
}

//=============================================================================
// Name: CAALEvent
// Description: Destructor
//=============================================================================
void CAALEvent::Delete()
{
   delete this;
}

//=============================================================================
// Name:          CAALEvent::UpdateContext
// Description:   Update m_Context based on m_pObject, cache the object context
// Interface:     protected
// Inputs:        object member m_pObject
// Outputs:       void
// Comments:      Utility function used for internal state maintenance
//=============================================================================
void CAALEvent::UpdateContext()
{
   if ( (NULL != m_pObject) && m_pObject->IsOK() ) {
      m_Context = m_pObject->Context();
   }
}

//=============================================================================
// Name: CAALEvent::SetInterface
// Description: Sets an interface pointer on the object.
// Interface: protected
// Inputs: Interface - name of the interface to set.
//         pInterface - Interface pointer
// Outputs: Interface pointer.
// Comments:
//=============================================================================
EOBJECT CAALEvent::SetInterface(btIID              Interface,
                                btGenericInterface pInterface) {
   AutoLock(this);
   // Make sure there is not an implementation already.
   if ( Has(Interface) ) {
      return EObjDuplicateName;
   }

   // Add the interface
   m_InterfaceMap[Interface] = pInterface;

   return EObjOK;
}

CAALEvent::CAALEvent() {/*empty*/}
CAALEvent & CAALEvent::operator=(const CAALEvent & ) { return *this; }

//=============================================================================
// Name: CAALEvent
// Description: Destructor
//=============================================================================
CAALEvent::~CAALEvent() {}

//=============================================================================
// Name: CTransactionEvent
// Description: constructor CTransactionEvent base class
// Comments: Must Initialize base to ensure Interface registration.
//           Must be constructed with an object and a TranID.
//=============================================================================
CTransactionEvent::CTransactionEvent(IBase               *pObject,
                                     TransactionID const &TranID,
                                     IMessageHandler     *pHandler) :
   CAALEvent(pObject, pHandler)
{
   AutoLock(this);
   m_TranID = TranID;

   // ITranEvent is the default native subclass interface unless overriden by a subclass
   if ( SetSubClassInterface(iidTranEvent, dynamic_cast<ITransactionEvent *>(this)) != EObjOK ) {
      return;
   }

   m_bIsOK = true;
}


//=============================================================================
// Name: CTransactionEvent
// Description: constructor CTransactionEvent base class
// Comments: Must Initialize base to ensure Interface registration.
//           Must be constructed with an object and a TranID.
//=============================================================================
CTransactionEvent::CTransactionEvent(IBase               *pObject,
                                     btIID                SubClassID,
                                     TransactionID const &TranID,
                                     IMessageHandler     *pHandler) :
   CAALEvent(pObject, pHandler)
{
   AutoLock(this);

   m_TranID = TranID;


   // ITranEvent is the default native subclass interface unless overriden by a subclass
   if ( SetInterface(iidTranEvent, dynamic_cast<ITransactionEvent *>(this)) != EObjOK ) {
      return;
   }

   // Set the subclass to the one provided
   if ( SetSubClassInterface(SubClassID, dynamic_cast<ITransactionEvent *>(this)) != EObjOK ) {
      return;
   }

   m_bIsOK = true;
}

//=============================================================================
// Name: CTransactionEvent
// Description: copy constructor CTransactionEvent base class
// Comments: Must Initialize base to ensure Interface registration.
//           Must be constructed with an object and a TranID.
//=============================================================================
CTransactionEvent::CTransactionEvent(CTransactionEvent const &rOther) :
   CAALEvent(rOther)
{
   AutoLock(this);

   // ITranEvent is the default native subclass interface unless overriden by a subclass
   if ( SetSubClassInterface(iidTranEvent, dynamic_cast<ITransactionEvent *>(this)) != EObjOK ) {
      m_bIsOK = false;
      return;
   }

   m_TranID = rOther.m_TranID;
}

//=============================================================================
// Name: CTransactionEvent
// Description: Destructor
//=============================================================================
CTransactionEvent::~CTransactionEvent() {}

CTransactionEvent::CTransactionEvent() {/*empty*/}
CTransactionEvent::CTransactionEvent(IBase * ) {/*empty*/}
CTransactionEvent & CTransactionEvent::operator=(const CTransactionEvent & ) { return *this; }

//=============================================================================
// Name: CExceptionEvent
// Description: Constructor
// Comments: Must Initialize base to ensure Interface registration.
//           Must be constructed with an object.
//=============================================================================
CExceptionEvent::CExceptionEvent(IBase    *pObject,
                                 btID      ExceptionNumber,
                                 btID      Reason,
                                 btcString Description,
                                 IMessageHandler *pHandler) :
   CAALEvent(pObject, pHandler),
   m_ExceptionNumber(ExceptionNumber),
   m_Reason(Reason),
   m_strDescription(Description)

{
   AutoLock(this);

   // default native subclass interface unless overriden by a subclass
   if ( SetSubClassInterface(iidExEvent, dynamic_cast<IExceptionEvent *>(this)) != EObjOK ) {
      return;
   }

   m_bIsOK = true;
}


//=============================================================================
// Name: CExceptionEvent
// Description: Constructor
// Comments: Must Initialize base to ensure Interface registration.
//           Must be constructed with an object.
//=============================================================================
CExceptionEvent::CExceptionEvent(IBase    *pObject,
                                 btIID     SubClassID,
                                 btID      ExceptionNumber,
                                 btID      Reason,
                                 btcString Description,
                                 IMessageHandler *pHandler) :
   CAALEvent(pObject, pHandler),
   m_ExceptionNumber(ExceptionNumber),
   m_Reason(Reason),
   m_strDescription(Description)
{
   AutoLock(this);

   // iidExEvent is the default native subclass interface unless overriden by a subclass
   if ( SetInterface(iidExEvent, dynamic_cast<IExceptionEvent *>(this)) != EObjOK ) {
      return;
   }

   // default native subclass interface unless overriden by a subclass
   if ( SetSubClassInterface(SubClassID, dynamic_cast<IExceptionEvent *>(this)) != EObjOK ) {
      return;
   }

   m_bIsOK = true;
}

//=============================================================================
// Name: CExceptionEvent
// Description: Copy Constructor
// Comments: Must Initialize base to ensure Interface registration.
//           Must be constructed with an object.
//=============================================================================
CExceptionEvent::CExceptionEvent(const CExceptionEvent &rOther) :
   CAALEvent(rOther),
   m_ExceptionNumber(rOther.m_ExceptionNumber),
   m_Reason(rOther.m_Reason),
   m_strDescription(rOther.m_strDescription)
{
   AutoLock(this);

   // default native subclass interface unless overriden by a subclass
   if ( SetSubClassInterface(iidExEvent, dynamic_cast<IExceptionEvent *>(this)) != EObjOK ) {
      m_bIsOK = false;
      return;
   }
}

//=============================================================================
// Name: ~CExceptionEvent
// Description: Destructor
//=============================================================================
CExceptionEvent::~CExceptionEvent() {}

CExceptionEvent::CExceptionEvent() {}
CExceptionEvent & CExceptionEvent::operator=(const CExceptionEvent & ) { return *this; }

//=============================================================================
// Name: CExceptionTransactionEvent
// Description: IMMExceptionTransactionEvent base class
// Comments: Must Initialize base to ensure Interface registration.
//           Must be constructed with an object and a TranID.
//=============================================================================
CExceptionTransactionEvent::CExceptionTransactionEvent(IBase               *pObject,
                                                       TransactionID const &TranID,
                                                       btID                 ExceptionNumber,
                                                       btID                 Reason,
                                                       btcString            Description,
                                                       IMessageHandler     *pHandler) :
   CAALEvent(pObject,pHandler),
   m_ExceptionNumber(ExceptionNumber),
   m_Reason(Reason),
   m_strDescription(Description)
{
   AutoLock(this);

   m_TranID = TranID;

   if ( SetInterface(iidTranEvent, dynamic_cast<ITransactionEvent *>(this)) != EObjOK ) {
      return;
   }

   // default native subclass interface unless overriden by a subclass
   if ( SetSubClassInterface(iidExTranEvent, dynamic_cast<IExceptionTransactionEvent *>(this)) != EObjOK ) {
      return;
   }

   m_bIsOK = true;
}


//=============================================================================
// Name: CExceptionTransactionEvent
// Description: CExceptionTransactionEvent base class
// Comments: Must Initialize base to ensure Interface registration.
//           Must be constructed with an object and a TranID.
//=============================================================================
CExceptionTransactionEvent::CExceptionTransactionEvent(IBase               *pObject,
                                                       btIID                SubClassID,
                                                       TransactionID const &TranID,
                                                       btID                 ExceptionNumber,
                                                       btID                 Reason,
                                                       btcString            Description,
                                                       IMessageHandler     *pHandler) :
   CAALEvent(pObject, pHandler),
   m_ExceptionNumber(ExceptionNumber),
   m_Reason(Reason),
   m_strDescription(Description)
{
   AutoLock(this);

   m_TranID = TranID;

   if ( SetInterface(iidTranEvent, dynamic_cast<ITransactionEvent *>(this)) != EObjOK ) {
      return;
   }

   if ( SetInterface(iidExTranEvent, dynamic_cast<IExceptionTransactionEvent *>(this)) != EObjOK ) {
      return;
   }

   // default native subclass interface unless overriden by a subclass
   if ( SetSubClassInterface(SubClassID, dynamic_cast<IExceptionTransactionEvent *>(this)) != EObjOK ) {
      return;
   }

   m_bIsOK = true;
}

//=============================================================================
// Name: CExceptionTransactionEvent
// Description: Copy Constructor
// Comments: Must Initialize base to ensure Interface registration.
//           Must be constructed with an object and a TranID.
//=============================================================================
CExceptionTransactionEvent::CExceptionTransactionEvent(CExceptionTransactionEvent const &rOther) :
   CAALEvent(rOther)
{
   AutoLock(this);

   if ( SetInterface(iidTranEvent, dynamic_cast<ITransactionEvent *>(this)) != EObjOK ) {
      m_bIsOK = false;
      return;
   }

   // default native subclass interface unless overriden by a subclass
   if ( SetSubClassInterface(iidExTranEvent, dynamic_cast<IExceptionTransactionEvent *>(this)) != EObjOK ) {
      m_bIsOK = false;
      return;
   }

   m_TranID          = rOther.m_TranID;
   m_strDescription  = rOther.m_strDescription;
   m_Reason          = rOther.m_Reason;
   m_ExceptionNumber = rOther.m_ExceptionNumber;
 }

//=============================================================================
// Name: ~CExceptionTransactionEvent
// Description: Destructor
//=============================================================================
CExceptionTransactionEvent::~CExceptionTransactionEvent() {}

CExceptionTransactionEvent::CExceptionTransactionEvent() {/*empty*/}
CExceptionTransactionEvent::CExceptionTransactionEvent(IBase * ) {/*empty*/}
CExceptionTransactionEvent & CExceptionTransactionEvent::operator=(const CExceptionTransactionEvent & ) { return *this; }



//=============================================================================
// Name: ObjectCreatedEvent
// Description: XL Runtime Event
//=============================================================================
ObjectCreatedEvent::ObjectCreatedEvent( IRuntimeClient       *prtClient,
                                        IServiceClient       *pClient,
                                        IBase                *pObject,
                                        TransactionID         TranID,
                                        const NamedValueSet  &OptArgs) :
   m_prtClient(prtClient),
   m_pClient(pClient),
   CTransactionEvent(pObject, TranID),
   m_OptArgs(OptArgs)
{
   SetSubClassInterface(tranevtFactoryCreate, dynamic_cast<IObjectCreatedEvent *>(this));
}

void ObjectCreatedEvent::operator()()
{
   // Notify the Runtime Client first
   if ( m_prtClient ) {
      m_prtClient->runtimeAllocateServiceSucceeded(m_pObject, m_TranID);
   }

   // Now notify the Service Client
   if ( NULL != m_pClient ) {
      m_pClient->serviceAllocated(m_pObject, m_TranID);
      delete this;
   } else if ( NULL != m_pMessageHandler ) {
      m_pMessageHandler->messageHandler(*this);
      delete this;
   } else if ( NULL != m_pEventHandler ) {
      m_pEventHandler(*this);
   }
}


ObjectCreatedEvent::~ObjectCreatedEvent() {/*empty*/}
ObjectCreatedEvent::ObjectCreatedEvent()  {/*empty*/}

ObjectCreatedExceptionEvent::ObjectCreatedExceptionEvent(IRuntimeClient     *prtClient,
                                                         IServiceClient     *pClient,
                                                         IBase              *pObject,
                                                         TransactionID       TranID,
                                                         btUnsigned64bitInt  ExceptionNumber,
                                                         btUnsigned64bitInt  Reason,
                                                         btcString           Description) :
   CExceptionTransactionEvent(pObject,
                              TranID,
                              ExceptionNumber,
                              Reason,
                              Description),
   m_prtClient(prtClient),
   m_pClient(pClient)
{
   m_SubClassID = extranevtFactoryCreate;
}

void ObjectCreatedExceptionEvent::operator()()
{
   // Notify the Runtime Client
   if(m_prtClient){
      m_prtClient->runtimeAllocateServiceFailed(*this);
      delete this;
   }else if(NULL != m_pClient){
         m_pClient->serviceAllocateFailed(*this);
         delete this;

   } else if(NULL != m_pMessageHandler){
      m_pMessageHandler->messageHandler(*this);
      delete this;
   }else if(NULL != m_pEventHandler){
      m_pEventHandler(*this);
   }
}

ObjectCreatedExceptionEvent::~ObjectCreatedExceptionEvent() {/*empty*/}
ObjectCreatedExceptionEvent::ObjectCreatedExceptionEvent()  {/*empty*/}

CObjectDestroyedTransactionEvent::CObjectDestroyedTransactionEvent(IServiceClient       *pClient,
                                                                   IBase                *pObject,
                                                                   TransactionID const  &TransID,
                                                                   btApplicationContext  Context) :
   m_pClient(pClient),
   CTransactionEvent(pObject, TransID)
{
   m_SubClassID = tranevtObjectDestroyed;
   m_Context    = Context;
}

CObjectDestroyedTransactionEvent::CObjectDestroyedTransactionEvent(const CObjectDestroyedTransactionEvent &rOther) :
   CTransactionEvent(dynamic_cast<CTransactionEvent const&>(rOther))
{
   m_SubClassID = tranevtObjectDestroyed;
   m_Context    = rOther.m_Context;
}

void CObjectDestroyedTransactionEvent::operator()()
{
   if(NULL != m_pClient){
      m_pClient->serviceFreed(m_TranID);;
      delete this;
   }else if(NULL != m_pMessageHandler){
      m_pMessageHandler->messageHandler(*this);
      delete this;
   }else if(NULL != m_pEventHandler){
      m_pEventHandler(*this);
   }
}
CObjectDestroyedTransactionEvent::~CObjectDestroyedTransactionEvent() {/*empty*/}
CObjectDestroyedTransactionEvent::CObjectDestroyedTransactionEvent()  {/*empty*/}



//=============================================================================
// Name: CApplicationEvent
// Description: Constructor
// Comments: Must Initialize base to ensure Interface registration.
//           Must be constructed with an object.
//=============================================================================
CApplicationEvent::CApplicationEvent(btIID          subclass,
                                     IBase         *pObject,
                                     NamedValueSet &NValues) :
   CAALEvent(pObject),
   m_subclass(subclass)
{
   AutoLock(this);

#if DEPRECATED
   //Self register the interface
   if(SetInterface(I_APPLICATION_EVENT,
                   dynamic_cast<IApplicationEvent *>(this) )!= EObjOK) {
      return;
   }

   //Self register the interface
   if(SetInterface(C_APPLICATION_EVENT,
                   dynamic_cast<C_ApplicationEvent*>(this) )!= EObjOK) {
      return;
   }

   //Register the application defined type
   if(SetInterface(subclass,
                   dynamic_cast<IApplicationEvent *>(this) )!= EObjOK) {
      return;
   }

   m_subclass = _strdup(subclass); // keep it for copy constructor
   m_pObject=pObject;
#endif // DEPRECATED

   //Event inherits object's Application Context Value
   if(pObject!=NULL && pObject->IsOK()) {
      m_Context=pObject->Context();
   }

   m_pNamedValues = new NamedValueSet(NValues);
   m_bIsOK = true;
}

//=============================================================================
// Name: CApplicationEvent(const CApplicationEvent&rOther)
// Description: Copy Constructor
// Comments: Must Initialize base to ensure Interface registration.
//           Must be constructed with an object.
//=============================================================================
CApplicationEvent::CApplicationEvent(const CApplicationEvent &rOther) :
   CAALEvent(rOther),
   m_subclass(rOther.m_subclass)
{
   AutoLock(this);
#if DEPRECATED
   //Self register the interface
   if(SetInterface(I_APPLICATION_EVENT,
                   dynamic_cast<IApplicationEvent *>(this) )!= EObjOK) {
      return;
   }

   if(SetInterface(C_APPLICATION_EVENT,
                   dynamic_cast<C_ApplicationEvent*>(this) )!= EObjOK) {
      return;
   }

   //Register the application defined type
   if(SetInterface(rOther.m_subclass,
                   dynamic_cast<IApplicationEvent *>(this) )!= EObjOK) {
      return;
   }

   m_pObject=rOther.m_pObject;
   m_subclass = _strdup(rOther.m_subclass);
#endif // DEPRECATED

   m_pNamedValues = new NamedValueSet(*rOther.m_pNamedValues);
   m_bIsOK = true;
}

//=============================================================================
// Name: ~CApplicationEvent()
// Description: Destructor
// Comments:
//=============================================================================
CApplicationEvent::~CApplicationEvent() {
#if DEPRECATED
   if ( NULL != m_subclass ) {
      free(m_subclass);
   }
#endif // DEPRECATED
   if ( NULL != m_pNamedValues ) {
      delete m_pNamedValues;
   }
}

CApplicationEvent::CApplicationEvent() {/*empty*/}
CApplicationEvent & CApplicationEvent::operator=(const CApplicationEvent & ) { return *this; }

END_NAMESPACE(AAL)


