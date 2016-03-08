// Copyright(c) 2014-2016, Intel Corporation
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
// **************************************************************************
/* 
 * Module Info: ASE Error reporting functions
 * Language   : System{Verilog} | C/C++
 * Owner      : Rahul R Sharma
 *              rahul.r.sharma@intel.com
 *              Intel Corporation
 */ 

#include "ase_common.h"

// -----------------------------------------------------------------------
// ASE error report : Prints a verbose report on catastrophic errors
// -----------------------------------------------------------------------
void ase_error_report(char *err_func, int err_num, int err_code)
{
  BEGIN_RED_FONTCOLOR;
  //  backtrace_wrapper();

  // Report error
  printf("@ERROR in %s CODE %d | %s\n", err_func, err_num, strerror(err_num) );
    
  // Corrective actions
  switch (err_code)
    {
      // CAPCM not initialized
    case ASE_USR_CAPCM_NOINIT:
      printf("QPI-CA private memory has not been initialized.\n");
      break;
      
      // Message queue error
    case ASE_OS_MQUEUE_ERR:
      printf("There was an error in the POSIX Message Queue subsystem.\n");
      printf("Please look up 'man mq_overview' for more information.\n");
      break;
      
      // Message queue error
    case ASE_OS_SHM_ERR:
      printf("There was an error in the POSIX Shared Memory subsystem.\n");
      break;

      // File open error
    case ASE_OS_FOPEN_ERR:
      printf("File opening failed. This could be due to several reasons: \n");
      printf("1. ASE is being run from the wrong relative paths, and causing fstat to fail.\n");
      printf("2. File system permissions are not optimal\n");
      break;
      
      // Memory map/unmap failed
    case ASE_OS_MEMMAP_ERR:
      printf("A problem occured when mapping or unmapping a memory region to a virtual base pointer.\n");
      break;

      // MQ send/receive error
    case ASE_OS_MQTXRX_ERR:
      printf("There was a problem when sending/receiving messages using the POSIX message queues.\n");
      printf("This may be due to sub-optimal message queue attributes.\n");
      break;

      // Malloc error
    case ASE_OS_MALLOC_ERR:
      printf("There was a problem with memory allocation system, NULL was returned.\n");
      printf("Simulator will attempt to close down.\n");
      break;

      // IPCkill catastrophic error
    case ASE_IPCKILL_CATERR:
      printf("There was an ERROR when trying to open IPC local listfile for cleaning.\n");
      printf("fopen failed, see .ase_ipc_local file, and clean them manually.\n");
      break;

      // Default or unknown error
    default:
      printf("ERROR code is not defined, or cause is unknown.\n");
      printf("If your agreement allows this, please report detailed steps to recreate the error to the developer.\n");
    }

  END_RED_FONTCOLOR;
}


/*
 * Wrapper for backtrace wrapper
 * Useful for debugging broken symbols
 */
void backtrace_wrapper()
{
  /* int bt_j, bt_nptrs;   */
  /* void *bt_buffer[4096]; */
  /* char **bt_strings; */

  bt_nptrs = backtrace(bt_buffer, 4096);
  printf("backtrace() returned %d addresses\n", bt_nptrs);

  bt_strings = backtrace_symbols(bt_buffer, bt_nptrs);
  if (bt_strings == NULL) {
    perror("backtrace_symbols");
    exit(EXIT_FAILURE);
  }

  for (bt_j = 0; bt_j < bt_nptrs; bt_j++)
    printf("%s\n", bt_strings[bt_j]);

  free(bt_strings);
}
