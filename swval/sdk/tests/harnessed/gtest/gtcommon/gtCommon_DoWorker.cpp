// INTEL CONFIDENTIAL - For Intel Internal Use Only
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif   // HAVE_CONFIG_H
#include "gtCommon.h"

/// ===================================================================
/// @internal        CMockDoWorker
///
btBool CMockDoWorker::init( IBase* pClientBase,
                            NamedValueSet const& optArgs,
                            TransactionID const& rtid )
{

   m_pWorkClient = dynamic_ptr
      <IMockWorkClient>( iidMockWorkClient, pClientBase );
   m_pSvcClient = dynamic_ptr<IServiceClient>( iidServiceClient, pClientBase );

   if ( NULL == m_pWorkClient || NULL == m_pSvcClient ) {

      initFailed( new CExceptionTransactionEvent( NULL,
                                                  rtid,
                                                  errBadParameter,
                                                  reasMissingInterface,
                                                  "Client did not "
                                                  "publish "
                                                  "IMockWorkClient "
                                                  "Interface" ) );
      return false;
   }

   initComplete( rtid );
   return true;
}

AALServiceModule* CMockDoWorker::getAALServiceModule() const
{
   return InModuleSvcsFact<CMockDoWorker>::GetServiceModule();
}

IServiceClient* CMockDoWorker::getServiceClient() const
{
   return dynamic_cast<IServiceClient*>( m_pSvcClient );
}

// requires a visiting worker, else not implemented
// unless overridden in a derived class
void CMockDoWorker::doWork()
{
   if ( NULL != m_pVisitingWorker ) {
      m_pVisitingWorker->doContractWork( this );
   } else {
      MSG( "!!!!!!!!!!!NOT IMPLEMENTED!!!!!!!!!!" );
   }
   dispatchWorkComplete( TransactionID() );
}

void CMockDoWorker::dispatchWorkComplete( TransactionID const& rTranID )
{
   ASSERT( NULL != m_pWorkClient );
   if ( m_pWorkClient != NULL ) {
      CMockDispatchable* pDisp = new ( std::nothrow ) CMockDispatchable(
         m_pWorkClient,
         static_cast<IBase*>( static_cast<CMockDoWorker*>( this ) ),
         rTranID );
      ASSERT( NULL != pDisp );
      if ( pDisp != NULL ) {
         getRuntime()->schedDispatchable( pDisp );
      }
   }
}

void CMockDoWorker::acceptVisitor( IVisitingWorker* pVisitor )
{
   ASSERT( NULL != pVisitor );
   if ( pVisitor != NULL ) {
      m_pVisitingWorker = pVisitor;
   }
}

btBool CMockDoWorker::Release( TransactionID const& rTranID, btTime timeout )
{
   return EmptyServiceBase::Release( rTranID, timeout );
}

btBool CMockDoWorker::initComplete( TransactionID const& rtid )
{
   return EmptyServiceBase::initComplete( rtid );
}
