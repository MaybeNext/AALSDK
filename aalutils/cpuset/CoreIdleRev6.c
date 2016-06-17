//
// Copyright(c) 2015-2016, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   andor other materials provided with the distribution.
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
// @file CoreIdleRev6
// @brief Standalone Test code for core idling
// @verbatim
// Not for production, not part of AAL.  It's only use is to provide later
// leverage when adding core idling to PR.
// DO NOT DISTRIBUTE!!!!!  NO or NON-ROBUST ERROR HANDLING!!!!!!
// To build use: gcc -o rev6 CoreIdleRev6.c -lm
// To use: sudo ./rev5 fpgapower
// Example: to specify 95 watts for the fpga
// Use: sudo ./rev5 95
// 
// AUTHORS: Carl Ohgren, Intel Corporation
//
//
// HISTORY:  Based on CoreIdleRev5.c. Uses sys_call to invokde sched_setaffinity
// to idle cores based on command line arguement that supplies the fpga power
// requirement.  Skips, Uses following order:
//   1.) Determine FPGA power
//   2.) Determine Available watts
//   3.) Set affinity mask for pids 1 and 2.
//   4.) Change for this rev... just pid_max number from /proc/sys/kernel/pid_max file.
//   5.) Set affinity mask for list
//    
// Again this for test purposes only.
// WHEN:          WHO:     WHAT:
// 05/31/2016     CGO      syscall(sched_setaffinity) leverage version.@endverbatim
//
//****************************************************************************
#ifndef __USE_GNU
#define __USE_GNU
#endif
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <sched.h>
#include <unistd.h>

int main(int argc, char *argv[])  
{
  FILE *fp;
  FILE *fp1;
  FILE *fp2;
  char data[1024];
  char data1[1024];
  char cmd[1024];
  char MaxThreadValStr[20];
  char *endptr;
  int64_t PackPwrLimit1;
  int64_t PowerUnitValue;
  long double FpgaWatts;
  int64_t PackagePowerUnit;
  long double TotalWatts;
  long double AvailableWatts;
  int64_t CoreCount;
  int64_t MaxThreadVal;
  int i, pid, max_pid_index;
  cpu_set_t set;
  int pid_list[4096];
  
  CPU_ZERO(&set);

  if (argc != 2) {
    printf("Usage rev1 watts\n");
    exit(1);
  }
  printf("echo argv1: %s\n", argv[1]);
  FpgaWatts = strtold(argv[1], &endptr);
  printf("Begin fpga watts: %Lf\n", FpgaWatts);
//
// "Kernel Mode Code"
// Open the rdmsr command
//
  fp = popen("/usr/sbin/rdmsr -c0 -p 0 0x610", "r");
  if (fp == NULL) {
    printf("Failed to run command\n");
    exit(1);
  }
  while (fgets(data, sizeof(data)-1, fp) != NULL) {
    printf("%s", data);
  }
  PackPwrLimit1 = strtoll(&data[2], &endptr, 16);
  printf("Power Limit converted: %llx \n", PackPwrLimit1);
  close(fp);
  PackPwrLimit1 = PackPwrLimit1 & 0x07fff;

  fp = popen("/usr/sbin/rdmsr -c0 -p 0 0x606", "r");
  while (fgets(data, sizeof(data)-1, fp) != NULL) {
    printf("%s", data);
  }
  PackagePowerUnit = strtoll(&data[2], &endptr, 16);
  printf("Package Power Unit Value converted: %llx \n", PackagePowerUnit);
  close(fp);  
  PowerUnitValue = PackagePowerUnit & 0x0f;
  PowerUnitValue = pow(2, PowerUnitValue);
  printf("Divisor of Raw Limit1:%llx\n", PowerUnitValue);
  TotalWatts = ((double)PackPwrLimit1)/((double)PowerUnitValue);
  printf("Total Watts: %Lf \n", TotalWatts);
  AvailableWatts = TotalWatts - (double)FpgaWatts;
  printf("Available Watts: %Lf\n", AvailableWatts);
  CoreCount = (int64_t) AvailableWatts / (int64_t) 5;  
  printf("Core Count: %Ld\n", CoreCount);
  MaxThreadVal = CoreCount * 2;
  //MaxThreadVal = MaxThreadVal - 1;
  snprintf(MaxThreadValStr, 20, "%d", MaxThreadVal);
  printf("Max Thread String Value: %s \n",MaxThreadValStr);
  
  for (i = 0; i < MaxThreadVal; i++) {
    CPU_SET(i, &set);
  }
  i = CPU_COUNT_S(sizeof(cpu_set_t), &set);
  printf("Cpu Count: %d \n",i);

//
// Kernel Mode Code
// Set affinity for pid 1 and pid 2, first.  Theory is that
// all children of pids created after these call inheirit affinity.
// Pids that already exist will get affinity from max_pid loop.
// This extra step may not be necessary when using max_pid loop.
//
  if (syscall(203, 1, sizeof(set), &set) == -1){
    printf("schedaffnity failure via syscall for pid: 1\n");
  }
  if (syscall(203, 2, sizeof(set), &set) == -1){
    printf("schedaffnity failure via syscall for pid: 2\n");
  }
//
// "User Mode Code"
// Find max pid number
  fp2 = fopen("/proc/sys/kernel/pid_max", "r");
  for (i = 0; i < 20; i++) {
    data1[i] = fgetc(fp2);
    if  (feof(fp2)) {
      data1[i] = 0;
      break;
    }
  }
  max_pid_index = strtol(&data1[0], &endptr, 10);
  close(fp2);


//
//  Set affinity for all possible pids to mask in cpuset.
//
  for (i = 0; i < max_pid_index; i++) {
    pid = i;
    if ((pid  == 1) || (pid == 2)) continue;   
    if (syscall(203,pid, sizeof(set), &set) == -1){
//      printf("schedaffnity failure via syscall for pid: %d\r", pid);
    }
  }  
  return 0; 
}
