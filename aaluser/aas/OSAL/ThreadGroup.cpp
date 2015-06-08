// Copyright (c) 2003-2015, Intel Corporation
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
/// @file ThreadGroup.cpp
/// @brief Implementation of the ThreadGroup class
/// @ingroup OSAL
/// @verbatim
/// Intel(R) QuickAssist Technology Accelerator Abstraction Layer
///
/// AUTHORS: Joseph Grecco, Intel Corporation
///          Henry Mitchel, Intel Corporation
///          Tim Whisonant, Intel Corporation
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 12/16/2007     JG       Changed include path to expose aas/
/// 05/08/2008     HM       Cleaned up windows includes
/// 05/08/2008     HM       Comments & License
/// 01/04/2009     HM       Updated Copyright
/// 03/06/2014     JG       Complete rewrite
/// 05/07/2015     TSW      Complete rewrite@endverbatim
//****************************************************************************
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif // HAVE_CONFIG_H

#include "aalsdk/osal/ThreadGroup.h"
#include "aalsdk/osal/Sleep.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//=============================================================================
// Name: OSLThreadGroup
// Description: Constructor
// Interface: public
// Inputs: uiMinThreads - Minimum number of threads to use (default = 0 = auto).
//         uiMaxThreads - Maximum threads. (default = 0 = auto)
//         nPriority - Thread priority
// Outputs: none.
// Comments: Setting min == max != 0 results in a static thread pool.
//           The algorithm summary:
//               - The work queue and its semaphore are initialized to zero
//                 for all threads to start.
//               - The number of worker threads is determined and
//                 the threads are created.
//               - A count-up semaphore is use to wait for workers to start
//=============================================================================
OSLThreadGroup::OSLThreadGroup(AAL::btUnsignedInt        uiMinThreads,
                               AAL::btUnsignedInt        uiMaxThreads,
                               OSLThread::ThreadPriority nPriority,
                               AAL::btTime               JoinTimeout) :
   m_bDestroyed(false),
   m_JoinTimeout(JoinTimeout),
   m_pState(NULL)
{
   // If Min Threads is zero then determine a good number based on
   //  configuration
   if ( 0 == uiMinThreads ) {
      // TODO Use GetNumProcessors(), eventually.
       // m_nNumThreads = GetNumProcessors();
       uiMinThreads = 1;
   }

//TODO implement MaxThreads and dynamic sizing
   if ( uiMaxThreads < uiMinThreads ) {
      uiMaxThreads = uiMinThreads;
   }

   // Create the State object. The state object is a standalone object
   //  whose life is somewhat independent of the ThreadGroup.  This is to
   //  allow for the case that ThreadGroup is destroyed before the worker threads
   //  have been deleted. By making the state and synchronization members outside
   //  the ThreadGroup, the Threads can safely access them even if the Group object
   //  is gone.
   m_pState = new(std::nothrow) OSLThreadGroup::ThrGrpState(uiMinThreads);
   if ( NULL == m_pState ) {
      m_bDestroyed = true;
      ASSERT(false);
      return;
   }

   ASSERT(m_pState->IsOK());
   if ( !m_pState->IsOK() ) {
      m_bDestroyed = true;
      delete m_pState;
      m_pState = NULL;
      return;
   }

   // Create the workers.

   AAL::btUnsignedInt i;
   for ( i = 0 ; i < uiMinThreads ; ++i ) {
      // TODO: if worker thread creation fails.
      CreateWorkerThread(OSLThreadGroup::ExecProc, nPriority, m_pState);
   }

   // Wait for all of the works to signal started.
   // TODO Add timeout.
   WaitForAllWorkersToStart(AAL_INFINITE_WAIT);
}

AAL::btBool OSLThreadGroup::Destroy(AAL::btTime Timeout)
{
   Lock();

   if ( m_bDestroyed ) {
      Unlock();
      return true;
   }

   m_bDestroyed = true;

   Unlock();

   return m_pState->Destroy(Timeout);
}

//=============================================================================
// Name: ~OSLThreadGroup
// Description: Destructor - Stop all threads.
// Interface: public
// Comments: The state of the dispatch queue is not deterministic.
//           Normally one would stop each thread AND wait for them to end with
//           a join, however ThreadGroup can be killed from within a Thread in
//           in the Group itself. So best that can be done for now is to have
//           each thread delete itself.
//=============================================================================
OSLThreadGroup::~OSLThreadGroup()
{
   if ( !Destroy(m_JoinTimeout) ) {
      ;
   }
}

//=============================================================================
// Name: ExecProc
// Description: Worker Thread entry point
// Interface: private
// Comments:
//=============================================================================
void OSLThreadGroup::ExecProc(OSLThread *pThread, void *lpParms)
{
   OSLThreadGroup::ThrGrpState *pState = reinterpret_cast<OSLThreadGroup::ThrGrpState *>(lpParms);

   ASSERT(NULL != pState);
   if ( NULL == pState ) {
      ASSERT(false);
      return;
   }

   // Notify the constructor that we are up.
   pState->WorkerHasStarted(pThread);

   OSLThreadGroup::ThrGrpState::eState state;

   IDispatchable *pWork;
   AAL::btBool    bRunning = true;

   while ( bRunning ) {

      pWork = NULL;
      state = pState->GetWorkItem(pWork);

      switch ( state ) {

         case OSLThreadGroup::ThrGrpState::Joining : {
            if ( NULL == pWork ) {
               // Queue has emptied - we are done.
               bRunning = false;
            } else {
               (*pWork) (); // invoke the functor via operator() ()
            }
         } break;

         case OSLThreadGroup::ThrGrpState::Draining : // FALL THROUGH
         case OSLThreadGroup::ThrGrpState::Running  : {
            if ( NULL != pWork ) {
               (*pWork) (); // invoke the functor via operator() ()
            }
         } break;

         case OSLThreadGroup::ThrGrpState::Stopped : {
            // don't dispatch any items
            if ( NULL != pWork ) {
               ASSERT(false);
               delete pWork;
            }
         } break;

         default : // keep looping
            break;
      }

   }

   pState->WorkerHasExited(pThread);
}

////////////////////////////////////////////////////////////////////////////////
// OSLThreadGroup::ThrGroupState

OSLThreadGroup::ThrGrpState::ThrGrpState(AAL::btUnsignedInt NumThreads) :
   m_eState(Running),
   m_Flags(THRGRPSTATE_FLAG_OK),
   m_WorkSemTimeout(AAL_INFINITE_WAIT),
   m_Joiner(0),
   m_ThrStartBarrier(),
   m_ThrJoinBarrier(),
   m_ThrExitBarrier(),
   m_WorkSem(),
   m_workqueue(),
   m_RunningThreads(),
   m_ExitedThreads(),
   m_DrainManager(this)
{
   if ( !m_ThrStartBarrier.Create(NumThreads) ) {
      flag_clrf(m_Flags, THRGRPSTATE_FLAG_OK);
   }

   if ( !m_ThrJoinBarrier.Create(1) ) {
      flag_clrf(m_Flags, THRGRPSTATE_FLAG_OK);
   }

   if ( !m_ThrExitBarrier.Create(NumThreads) ) {
      flag_clrf(m_Flags, THRGRPSTATE_FLAG_OK);
   }

   if ( !m_WorkSem.Create(0, INT_MAX) ) {
      flag_clrf(m_Flags, THRGRPSTATE_FLAG_OK);
   }
}

OSLThreadGroup::ThrGrpState::~ThrGrpState()
{
   ASSERT(Joining == m_eState);
   ASSERT(m_workqueue.empty());
   ASSERT(m_RunningThreads.empty());
   ASSERT(m_ExitedThreads.empty());
   DestructMembers();
}

//=============================================================================
// Name: GetNumThreads
// Description: Get Number of Threads
// Interface: public
// Comments:
//=============================================================================
AAL::btUnsignedInt OSLThreadGroup::ThrGrpState::GetNumThreads() const
{
   AutoLock(this);
   return (AAL::btUnsignedInt) m_RunningThreads.size();
}

//=============================================================================
// Name: GetNumWorkItems
// Description: Get Number of workitems
// Interface: public
// Comments:
//=============================================================================
AAL::btUnsignedInt OSLThreadGroup::ThrGrpState::GetNumWorkItems() const
{
   AutoLock(this);
   return (AAL::btUnsignedInt) m_workqueue.size();
}

//=============================================================================
// Name: Add
// Description: Submits a work object for disposition
// Interface: public
// Inputs: pwi - pointer to work item object.
// Outputs: none.
// Comments: Object gets placed on the dispatch queue where it gets picked up
//           by the next available thread.
//=============================================================================
AAL::btBool OSLThreadGroup::ThrGrpState::Add(IDispatchable *pDisp)
{
   ASSERT(NULL != pDisp);
   if ( NULL == pDisp ) {
      return false;
   }

   Lock();

   const eState state = State();

   // We allow new work items when Running or Joining.
   if ( ( Stopped  == state ) ||
        ( Draining == state ) ) {
      Unlock();
      return false;
   }

   m_workqueue.push(pDisp);

   Unlock();

   // Signal the semaphore outside the critical section so that waking threads have an
   // opportunity to immediately acquire it.
   m_WorkSem.Post(1);

   return true;
}

//=============================================================================
// Name: Stop
// Description: Stop all threads.
// Interface: public
// Input: flush - If true will cause all messages to be destroyed before
//        return. Some items may not be dispatched
// Comments:
//=============================================================================
void OSLThreadGroup::ThrGrpState::Stop()
{
   AutoLock(this);

   if ( Stopped != State(Stopped) ) {
      // State conflict, can't stop right now.
      return;
   }

   // We're flushing all current work items. This Reset() will fail if there are currently
   // blockers on m_WorkSem; but if workers are blocked, then the work queue must be empty,
   // which is what we want anyway - no need to check the return value from Reset().
   m_WorkSem.Reset(0);

   // If there is something on the queue then remove it and destroy it.
   while ( m_workqueue.size() > 0 ) {
      IDispatchable *wi = m_workqueue.front();
      m_workqueue.pop();
      delete wi;
   }
}

//=============================================================================
// Name: Start
// Description: Restart the Thread Group
// Interface: public
// Comments:
//=============================================================================
AAL::btBool OSLThreadGroup::ThrGrpState::Start()
{
   AutoLock(this);

   if ( Running == State(Running) ) {
      AAL::btInt s = (AAL::btInt) m_workqueue.size();

      AAL::btInt c = 0;
      AAL::btInt m = 0;
      m_WorkSem.CurrCounts(c, m);

      if ( c >= s ) {
         // Nothing to do.
         return true;
      }

      return m_WorkSem.Post(s - c);
   }

   return false;
}

// Do a state transition.
OSLThreadGroup::ThrGrpState::eState OSLThreadGroup::ThrGrpState::State(eState st)
{
   AutoLock(this);

   if ( Joining == m_eState ) {
      // Joining is a final state. Deny all requests to do otherwise.
      return m_eState;
   }

   switch ( m_eState ) {
      case Running : {
         switch ( st ) {
            /* case Running :  break; */ // Running -> Running (nop)
            case Stopped  : m_eState = st; break; // Running -> Stopped  [ Stop()  ]
            case Draining : m_eState = st; break; // Running -> Draining [ Drain() ]
            case Joining  : m_eState = st; break; // Running -> Joining  [ Join()  ]
         }
      } break;

      case Stopped : {
         switch ( st ) {
            case Running  : m_eState = st; break; // Stopped -> Running   [ Start() ]
            /* case Stopped : break; */ // Stopped -> Stopped (nop)
            case Draining : ASSERT(Draining != st); break; // Invalid (queue empty check)
            case Joining  : m_eState = st; break; // Stopped -> Joining   [ Join(), ~OSLThreadGroup() ]
         }
      } break;

      case Draining : {
        switch ( st ) {
            case Running  : m_eState = st; break; // Draining -> Running  [ Drain() ]
            case Stopped  : ASSERT(Stopped != st);  break; // Invalid
            /* case Draining : break; */ // Draining -> Draining (nop)
            case Joining  : m_eState = st; break; // Draining -> Joining  [ Join()  ]
         }
      } break;
   }

   return m_eState;
}

AAL::btBool OSLThreadGroup::ThrGrpState::CreateWorkerThread(ThreadProc                fn,
                                                            OSLThread::ThreadPriority pri,
                                                            void                     *context)
{
   OSLThread *pThread = new(std::nothrow) OSLThread(fn, pri, context);

   ASSERT(NULL != pThread);
   if ( NULL == pThread ) {
      return false;
   }

   ASSERT(pThread->IsOK());
   if ( !pThread->IsOK() ) {
      delete pThread;
      return false;
   }

   Lock();
   m_RunningThreads.push_back(pThread);
   Unlock();

   return true;
}

//=============================================================================
// Name: GetWorkItem
// Description: Get next work item and the current ThreadGroup state
// Interface: public
// Comments:
//=============================================================================
OSLThreadGroup::ThrGrpState::eState OSLThreadGroup::ThrGrpState::GetWorkItem(IDispatchable * &pWork)
{
   // Wait for work item
   m_WorkSem.Wait(m_WorkSemTimeout);

   // Lock until flag and queue have been processed
   Lock();

   const eState state = State();

   switch ( state ) {
      case Joining  : // FALL THROUGH
      case Draining : // FALL THROUGH
      case Running  : {
         if ( m_workqueue.size() > 0 ) {
            pWork = m_workqueue.front();
            m_workqueue.pop();
         }
      } break;

      case Stopped : {
         ; // return without setting pWork to a work item.
      } break;

      default : {
         // Invalid state.
         ASSERT(false);
         ;
      } break;
   }

   Unlock();

   return state;
}

OSLThread * OSLThreadGroup::ThrGrpState::ThreadRunningInThisGroup(AAL::btTID tid) const
{
   AutoLock(this);

   const_thr_list_iter iter;
   for ( iter = m_RunningThreads.begin() ; m_RunningThreads.end() != iter ; ++iter ) {
      if ( (*iter)->IsThisThread(tid) ) {
         return *iter;
      }
   }
   return NULL;
}

void OSLThreadGroup::ThrGrpState::WorkerHasStarted(OSLThread *pThread)
{
   m_ThrStartBarrier.Post(1);
}

AAL::btBool OSLThreadGroup::ThrGrpState::WaitForAllWorkersToStart(AAL::btTime Timeout)
{
   return m_ThrStartBarrier.Wait(Timeout);
}

void OSLThreadGroup::ThrGrpState::WorkerIsSelfTerminating(OSLThread *pThread)
{
   Lock();

   // Remove the worker from the Running list, without placing it in the Exited list, because
   //  we don't want to attempt to Join() this worker.
   thr_list_iter iter = std::find(m_RunningThreads.begin(), m_RunningThreads.end(), pThread);

   if ( m_RunningThreads.end() != iter ) {
      m_RunningThreads.erase(iter);
   }

   // When self-terminating, if pThread is also a self-Drain()'er, we would not
   //  otherwise be able to Post() the drain Barrier object. Do it here, instead.
   while ( m_DrainManager.End(pThread->tid(), NULL) ) {
      m_DrainManager.ForciblyCompleteWorkItem();
   }

   Unlock();

   // The worker has now "exited".
   m_ThrExitBarrier.Post(1);
}

void OSLThreadGroup::ThrGrpState::WorkerHasExited(OSLThread *pThread)
{
   Lock();

   // Move the worker from Running to Exited.
   thr_list_iter iter = std::find(m_RunningThreads.begin(), m_RunningThreads.end(), pThread);

   if ( m_RunningThreads.end() != iter ) {
      m_RunningThreads.erase(iter);
      m_ExitedThreads.push_back(pThread);
   }

   Unlock();

   m_ThrExitBarrier.Post(1);
}

AAL::btBool OSLThreadGroup::ThrGrpState::Quiesce(AAL::btTime Timeout)
{
   AAL::btBool res;

   // Wait for all thread group workers to Post() m_ThrExitBarrier, signaling that they
   //  are exiting.
   res = m_ThrExitBarrier.Wait(Timeout);
   ASSERT(res);
   if ( !res ) {
      return false;
   }

   // If we are the designated Join()'er, then Join() all workers. If not, then wait for
   //  the Join()'er to do its thing.

   const AAL::btTID MyThrID = GetThreadID();

   Lock();

   // We need the check on THRGRPSTATE_FLAG_JOINING, because we don't know if 0 is a valid tid.

   if ( flag_is_set(m_Flags, THRGRPSTATE_FLAG_JOINING) && ( MyThrID == m_Joiner ) ) {

      Unlock();

      // ASSERT: all workers have exited (we waited on m_ThrExitBarrier above).
      // ASSERT: we are the sole Join()'er

      thr_list_iter iter;
      for ( iter = m_ExitedThreads.begin() ; m_ExitedThreads.end() != iter ; ++iter ) {
         (*iter)->Join();
         delete *iter;
      }
      m_ExitedThreads.clear();

      // Are any external Drain()'ers blocked on our work item? When a self-referential Join() or
      // a self-referential Destroy() is allowed to progress when there is an external Drain()'er(s),
      // the thread group worker must signal the completion of the Drain() here, before
      // self-terminating. Otherwise, the external Drain()'ers will become deadlocked.
      m_DrainManager.ReleaseAllDrainers();

      m_DrainManager.WaitForAllDrainersDone();

      res = m_ThrJoinBarrier.Post(1);
      ASSERT(res);
      if ( !res ) {
         return false;
      }

   } else {
      Unlock();

      // We're not the Join()'er.

      res = m_ThrJoinBarrier.Wait(Timeout);
      ASSERT(res);
      if ( !res ) {
         return false;
      }

   }

   return true;
}

void OSLThreadGroup::ThrGrpState::DestructMembers()
{
   m_ThrStartBarrier.Destroy();
   m_ThrExitBarrier.Destroy();
   m_DrainManager.DestructMembers();
   m_ThrJoinBarrier.Destroy();
}

OSLThreadGroup::ThrGrpState::DrainManager::DrainManager(ThrGrpState *pTGS) :
   m_pTGS(pTGS),
   m_DrainNestLevel(0),
   m_WaitTimeout(AAL_INFINITE_WAIT)
{
   m_DrainerDoneBarrier.Create(1);
}

OSLThreadGroup::ThrGrpState::DrainManager::~DrainManager()
{
   ASSERT(0 == m_DrainNestLevel);
   ASSERT(0 == m_SelfDrainers.size());
   ASSERT(0 == m_NestedWorkItems.size());
}

Barrier * OSLThreadGroup::ThrGrpState::DrainManager::Begin(AAL::btTID tid, AAL::btUnsignedInt items)
{
   AutoLock(m_pTGS);

   ++m_DrainNestLevel;

   if ( 1 == m_DrainNestLevel ) {
      // Beginning a new series of (possibly nested) Drain() calls.
      ASSERT((AAL::btUnsignedInt) m_pTGS->m_workqueue.size() == items);
      ASSERT(0 == m_NestedWorkItems.size());

      m_DrainerDoneBarrier.Reset();

      m_DrainBarrier.Destroy();
      m_DrainBarrier.Create(items);

      work_queue_t        tmpq;
      IDispatchable      *pWork;
      NestedBarrierPostD *pNested;

      // Pull each item from the work queue, and wrap it in a NestedBarrierPostD() object.
      while ( m_pTGS->m_workqueue.size() > 0 ) {
         pWork = m_pTGS->m_workqueue.front();
         m_pTGS->m_workqueue.pop();

         pNested = new(std::nothrow) NestedBarrierPostD(pWork, this);
         m_NestedWorkItems.push_back(pNested);

         tmpq.push( pNested );
      }

      // Re-populate the work queue.
      while ( tmpq.size() > 0 ) {
         m_pTGS->m_workqueue.push(tmpq.front());
         tmpq.pop();
      }
   }

   // non-NULL means self-referential Drain().
   OSLThread *pThread = m_pTGS->ThreadRunningInThisGroup(tid);

   if ( NULL != pThread ) {
      // Add tid to the list of self-drainers, allowing duplicates.
      m_SelfDrainers.push_back(tid);
      // Self-drainers don't block on the Barrier.
      return NULL;
   }

   return &m_DrainBarrier;
}

void OSLThreadGroup::ThrGrpState::DrainManager::CompleteNestedWorkItem(OSLThreadGroup::ThrGrpState::DrainManager::NestedBarrierPostD *pItem)
{
   m_pTGS->Lock();

   nested_list_iter iter = std::find(m_NestedWorkItems.begin(), m_NestedWorkItems.end(), pItem);

   ASSERT(m_NestedWorkItems.end() != iter);
   if ( m_NestedWorkItems.end() != iter ) {
      m_NestedWorkItems.erase(iter);
   }

   m_pTGS->Unlock();

   delete pItem;
   m_DrainBarrier.Post(1);
}

AAL::btBool OSLThreadGroup::ThrGrpState::DrainManager::End(AAL::btTID  tid,
                                                           Barrier    *pDrainBarrier)
{
   AAL::btBool res = false;

   AutoLock(m_pTGS);

   if ( NULL == pDrainBarrier ) {
      // self-referential Drain(). Remove one instance of tid from m_SelfDrainers.

      drainer_list_iter iter;
      for ( iter = m_SelfDrainers.begin() ; m_SelfDrainers.end() != iter ; ++iter ) {
         if ( ThreadIDEqual(tid, *iter) ) {
            m_SelfDrainers.erase(iter);
            if ( m_DrainNestLevel > 0 ) {
               --m_DrainNestLevel;
            }
            res = true;
            break;
         }
      }

   } else {
      // external Drain().
      res = true;
      if ( m_DrainNestLevel > 0 ) {
         --m_DrainNestLevel;
      }
   }

   return res;
}

AAL::btBool OSLThreadGroup::ThrGrpState::DrainManager::ReleaseAllDrainers()
{
   nested_list_iter iter;
   for ( iter = m_NestedWorkItems.begin() ; iter != m_NestedWorkItems.end() ; ++iter ) {
      delete *iter;
   }
   m_NestedWorkItems.clear();
   m_WaitTimeout = 100;
   return m_DrainBarrier.UnblockAll();
}

void OSLThreadGroup::ThrGrpState::DrainManager::ForciblyCompleteWorkItem()
{
   m_DrainBarrier.Post(1);
}

void OSLThreadGroup::ThrGrpState::DrainManager::AllDrainersAreDone()
{
   m_DrainerDoneBarrier.Post(1);
}

void OSLThreadGroup::ThrGrpState::DrainManager::WaitForAllDrainersDone()
{
   m_pTGS->Lock();

   while ( m_DrainNestLevel > 0 ) {
      m_pTGS->Unlock();
      m_DrainerDoneBarrier.Wait(m_WaitTimeout);
      m_pTGS->Lock();
   }

   m_pTGS->Unlock();
}

void OSLThreadGroup::ThrGrpState::DrainManager::DestructMembers()
{
   m_DrainBarrier.Destroy();
   m_DrainerDoneBarrier.Destroy();
}

//=============================================================================
// Name: Drain
// Description: Synchronize with the execution of any work items currently in
//              the queue. All work items in the queue will be executed
//              before returning.
// Returns: true - success
//          false - Attempt to Drain from a member Thread
// Interface: public
// Comments:
//=============================================================================
AAL::btBool OSLThreadGroup::ThrGrpState::Drain()
{
   // Refer to the state checking mutator:
   //   eState OSLThreadGroup::ThrGrpState::State(eState st);
   //
   // The only valid transitions to state Draining are
   //   Running  -> Draining
   //   Draining -> Draining (nested Drain() calls)
   //
   // Allowing nested Drain()'s presents a problem - as the inner Drain()'s complete, they
   // must not set the state of the OSLThreadGroup to Running (this would allow Add()'s during
   // the outer Drain()'s, eg). The inner Drain()'s must leave the state set to Draining, and
   // only the last Drain() to complete can set the state back to Running.

   Lock();

   const AAL::btUnsignedInt items = (AAL::btUnsignedInt) m_workqueue.size();

   // No need to drain if already empty.
   if ( 0 == items ) {
      Unlock();
      return true;
   }

   // Check for other state conflicts.
   if ( Draining != State(Draining) ) {
      // Can't drain now - state conflict.
      Unlock();
      return false;
   }

   AAL::btTID     MyThrID       = GetThreadID();
   Barrier       *pDrainBarrier = m_DrainManager.Begin(MyThrID, items);
   IDispatchable *pWork;

   if ( NULL == pDrainBarrier ) {
      // Self-referential Drain().

      // We need to continue to execute work.
      while ( m_workqueue.size() > 0 ) {
         pWork = m_workqueue.front();
         m_workqueue.pop();
         Unlock();

         pWork->operator() ();

         Lock();
      }

   } else {
      // external Drain().

      Unlock();

      // Wait for the work items to complete. Don't wait while locked.
      pDrainBarrier->Wait();

      Lock();
   }

   eState st = State();

   // DrainManager::End() will unlock the critical section.
   m_DrainManager.End(MyThrID, pDrainBarrier);
   AAL::btUnsignedInt level = m_DrainManager.DrainNestLevel();

   if ( 0 == level ) {
      m_DrainManager.AllDrainersAreDone();
   }

   if ( Joining == st ) {
      // Join() during Drain() is allowed. In this case, an attempt to set the
      // state to Running or Draining below would be denied. We want the success indicator here,
      // however, to reflect that the thread group was drained of items, hence this check.
      Unlock();
      return true;
   }

   st = (0 == level) ? Running : Draining;

   AAL::btBool res = (State(st) == st);

   Unlock();

   return res;
}

AAL::btBool OSLThreadGroup::ThrGrpState::Join(AAL::btTime Timeout)
{
   Lock();

   const eState st = State();

   if ( Joining == st ) {
      // Prevent nested / multiple Join().
      Unlock();
      return false;
   }

   AAL::btBool      res;
   const AAL::btTID MyThrID = GetThreadID();
   OSLThread       *pThread = ThreadRunningInThisGroup(MyThrID);

   // We are now Joining.
   State(Joining);

   // Workers are no longer required to block infinitely on work items. Give them a
   // chance to wake periodically and see that the thread group is being joined.
   m_WorkSemTimeout = PollingInterval();

   // Wake any threads that happen to be blocked infinitely.
   m_WorkSem.Post( (AAL::btInt) m_RunningThreads.size() );

   // Claim the Join().
   ASSERT(flag_is_clr(m_Flags, THRGRPSTATE_FLAG_JOINING));
   m_Joiner = MyThrID;
   flag_setf(m_Flags, THRGRPSTATE_FLAG_JOINING);

   if ( NULL != pThread ) {
      // self-referential Join()
      flag_setf(m_Flags, THRGRPSTATE_FLAG_SELF_JOIN);

      // We need to continue to execute work.
      IDispatchable *pWork;
      while ( m_workqueue.size() > 0 ) {
         pWork = m_workqueue.front();
         m_workqueue.pop();
         Unlock();

         pWork->operator() ();

         Lock();
      }

      WorkerIsSelfTerminating(pThread);

      Unlock();

      res = Quiesce(Timeout);
      if ( !res ) {
         return false;
      }

      // self-destruct. This thread terminates.
      delete pThread;

      // We must not return from this call.
      ASSERT(false);
      return false;
   }

   // Not a self-referential Join().

   Unlock();

   res = Quiesce(Timeout);
   if ( !res ) {
      return false;
   }

   return true;
}

AAL::btBool OSLThreadGroup::ThrGrpState::Destroy(AAL::btTime Timeout)
{
   Lock();

   const eState st = State();

   if ( Draining == st ) {
      return DestroyWhileDraining(Timeout);
   }

   if ( Joining == st ) {
      return DestroyWhileJoining(Timeout);
   }

   // We're not Drain()'ing and we haven't Join()'ed workers, yet.

   AAL::btBool      res;
   const AAL::btTID MyThrID = GetThreadID();
   OSLThread       *pThread = ThreadRunningInThisGroup(MyThrID);

   // Workers are no longer required to block infinitely on work items. Give them a chance
   // to wake periodically and see that the thread group is being joined.
   m_WorkSemTimeout = PollingInterval();

   State(Joining);

   // Wake any threads that happen to be blocked infinitely.
   m_WorkSem.Post( (AAL::btInt) m_RunningThreads.size() );

   // Claim the Join().
   ASSERT(flag_is_clr(m_Flags, THRGRPSTATE_FLAG_JOINING));
   m_Joiner = MyThrID;
   flag_setf(m_Flags, THRGRPSTATE_FLAG_JOINING);

   if ( NULL != pThread ) {
      // Self-referential Destroy().

      flag_setf(m_Flags, THRGRPSTATE_FLAG_SELF_JOIN);

      IDispatchable *pWork;
      while ( m_workqueue.size() > 0 ) {
         pWork = m_workqueue.front();
         m_workqueue.pop();
         Unlock();

         pWork->operator() ();

         Lock();
      }

      WorkerIsSelfTerminating(pThread);

      Unlock();

      res = Quiesce(Timeout);
      if ( !res ) {
         return false;
      }

      delete this;

      // self-destruct - this thread will terminate.
      delete pThread;

      // We must not return from this fn.
      ASSERT(false);
      return false;
   }

   // Not a self-referential Destroy(), not Drain()'ing, first time Join()'ing.

   Unlock();

   res = Quiesce(Timeout);
   if ( !res ) {
      return false;
   }

   delete this;
   return true;
}

AAL::btBool OSLThreadGroup::ThrGrpState::DestroyWhileDraining(AAL::btTime Timeout)
{
   // ** We are still Lock()'ed from the initial call to Destroy().

   // Destroy while another thread is actively Drain()'ing.
   // We must wait for the Drain() to complete.

   // We know that a Drain() is actively in progress, because Drain() will transition the
   //  state back to Running or Joining when all Drain()'ers are finished.

   AAL::btBool      res;
   thr_list_iter    iter;
   const AAL::btTID MyThrID = GetThreadID();
   OSLThread       *pThread = ThreadRunningInThisGroup(MyThrID);

   // Workers are no longer required to block infinitely on work items. Give them a chance
   // to wake periodically and see that the thread group is being joined.
   m_WorkSemTimeout = PollingInterval();

   // Transition the state to Joining here. This is to prevent another thread from beginning
   //  a Join() once we unlock the critical section. Destroy() is essentially a Join(),
   //  and we don't allow Join()'s to nest.
   State(Joining);

   // Wake any threads that happen to be blocked infinitely.
   m_WorkSem.Post( (AAL::btInt) m_RunningThreads.size() );

   // Claim the Join().
   ASSERT(flag_is_clr(m_Flags, THRGRPSTATE_FLAG_JOINING));
   m_Joiner = MyThrID;
   flag_setf(m_Flags, THRGRPSTATE_FLAG_JOINING);

   if ( NULL != pThread ) {
      // self-referential Destroy() during Drain().

      flag_setf(m_Flags, THRGRPSTATE_FLAG_SELF_JOIN);

      // continue to execute work on behalf of the Drain().
      IDispatchable *pWork;
      while ( m_workqueue.size() > 0 ) {
         pWork = m_workqueue.front();
         m_workqueue.pop();
         Unlock();

         pWork->operator() ();

         Lock();
      }

      WorkerIsSelfTerminating(pThread);

      Unlock();

      res = Quiesce(Timeout);
      if ( !res ) {
         return false;
      }

      delete this;

      // Self-destruct - this thread will terminate.
      delete pThread;

      // We must not return from this call.
      ASSERT(false);
      return false;
   }

   Unlock();

   // non-self-referential Destroy() during Drain().

   res = Quiesce(Timeout);
   if ( !res ) {
      return false;
   }

   delete this;
   return true;
}

AAL::btBool OSLThreadGroup::ThrGrpState::DestroyWhileJoining(AAL::btTime Timeout)
{
   // ** We are still Lock()'ed from the initial call to Destroy().

   // Destroy while another thread may be actively Join()'ing.
   // We must wait for the Join() to complete.

   AAL::btBool      res;
   const AAL::btTID MyThrID = GetThreadID();
   OSLThread       *pThread = ThreadRunningInThisGroup(MyThrID);

   // Workers are no longer required to block infinitely on work items. Give them a chance
   // to wake periodically and see that the thread group is being joined.
   m_WorkSemTimeout = PollingInterval();

   // Wake any threads that happen to be blocked infinitely.
   m_WorkSem.Post( (AAL::btInt) m_RunningThreads.size() );

   if ( NULL != pThread ) {
      // Self-referential Destroy() during Join().

      // Continue to dispatch work on behalf of the Join().
      IDispatchable *pWork;
      while ( m_workqueue.size() > 0 ) {
         pWork = m_workqueue.front();
         m_workqueue.pop();
         Unlock();

         pWork->operator() ();

         Lock();
      }

      WorkerIsSelfTerminating(pThread);

      Unlock();

      res = Quiesce(Timeout);
      if ( !res ) {
         return false;
      }

      delete this;

      // Self-destruct - this thread will terminate.
      delete pThread;

      // We must not return from this call.
      ASSERT(false);
      return true;
   }

   // non-self-referential Destroy() during Join().

   Unlock();

   res = Quiesce(Timeout);
   if ( !res ) {
      return false;
   }

   delete this;
   return true;
}

