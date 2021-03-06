##******************************************************************************
## This  file  is  provided  under  a  dual BSD/GPLv2  license.  When using or
##         redistributing this file, you may do so under either license.
##
##                            GPL LICENSE SUMMARY
##
##  Copyright(c) 2008-2016, Intel Corporation.
##
##  This program  is  free software;  you  can redistribute it  and/or  modify
##  it  under  the  terms of  version 2 of  the GNU General Public License  as
##  published by the Free Software Foundation.
##
##  This  program  is distributed  in the  hope that it  will  be useful,  but
##  WITHOUT   ANY   WARRANTY;   without   even  the   implied   warranty    of
##  MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the   GNU
##  General Public License for more details.
##
##  The  full  GNU  General Public License is  included in  this  distribution
##  in the file called README.GPLV2-LICENSE.TXT.
##
##  Contact Information:
##  Henry Mitchel, henry.mitchel at intel.com
##  77 Reed Rd., Hudson, MA  01749
##
##                                BSD LICENSE
##
##  Copyright(c) 2008-2016, Intel Corporation.
##
##  Redistribution and  use  in source  and  binary  forms,  with  or  without
##  modification,  are   permitted  provided  that  the  following  conditions
##  are met:
##
##    * Redistributions  of  source  code  must  retain  the  above  copyright
##      notice, this list of conditions and the following disclaimer.
##    * Redistributions in  binary form  must  reproduce  the  above copyright
##      notice,  this  list of  conditions  and  the  following disclaimer  in
##      the   documentation   and/or   other   materials   provided  with  the
##      distribution.
##    * Neither   the  name   of  Intel  Corporation  nor  the  names  of  its
##      contributors  may  be  used  to  endorse  or promote  products derived
##      from this software without specific prior written permission.
##
##  THIS  SOFTWARE  IS  PROVIDED  BY  THE  COPYRIGHT HOLDERS  AND CONTRIBUTORS
##  "AS IS"  AND  ANY  EXPRESS  OR  IMPLIED  WARRANTIES,  INCLUDING,  BUT  NOT
##  LIMITED  TO, THE  IMPLIED WARRANTIES OF  MERCHANTABILITY  AND FITNESS  FOR
##  A  PARTICULAR  PURPOSE  ARE  DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT
##  OWNER OR CONTRIBUTORS BE LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,
##  SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL   DAMAGES  (INCLUDING,   BUT   NOT
##  LIMITED  TO,  PROCUREMENT  OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF USE,
##  DATA,  OR PROFITS;  OR BUSINESS INTERRUPTION)  HOWEVER  CAUSED  AND ON ANY
##  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT LIABILITY,  OR TORT
##  (INCLUDING  NEGLIGENCE  OR OTHERWISE) ARISING  IN ANY WAY  OUT  OF THE USE
##  OF  THIS  SOFTWARE, EVEN IF ADVISED  OF  THE  POSSIBILITY  OF SUCH DAMAGE.
##******************************************************************************
##****************************************************************************
##  FILE: aalkernel/aalresourcemgr_server/Makefile
##     Accelerator Abstraction Layer (AAL)
##     AAL Resource Manager Server
##        Driver Module for Linux
##  Author:
##     Tim Whisonant, Intel Corporation
##  Created:
##     08/15/2014
##****************************************************************************
MY_KBUILD_VERBOSE    = $(my__kbuild_verbose_$(V))
my__kbuild_verbose_  = $(AM_DEFAULT_VERBOSITY)
my__kbuild_verbose_0 = 0
my__kbuild_verbose_1 = 1

moduledir=@AAL_OS_MODULE_INSTALL_PATH@

module_DATA=\
kbuild/@AALRMS_DRV_NAME@.ko

EXTRA_DIST=Kbuild.in

EXTRA_PROGRAMS=driver

driver_SOURCES=\
aalrm_server-file.c \
aalrm_server-int.h \
aalrm_server-ioctl.c \
aalrm_server-main.c \
aalrm_server-services.c \
aalrms-events-int.h

kbuild/@AALRMS_DRV_NAME@.ko: kbuild/Module.symvers

kbuild/Module.symvers: $(abs_top_builddir)/aalbus/kbuild/Module.symvers $(driver_SOURCES) $(abs_top_builddir)/aalresourcemgr_server/kbuild/kOSAL.c $(abs_top_builddir)/aalresourcemgr_server/kbuild/Makefile
	@cp --preserve=all "$(abs_top_builddir)/aalbus/kbuild/Module.symvers" "$(abs_top_builddir)/aalresourcemgr_server/kbuild" && \
$(MAKE) -C "@AAL_OS_KERNEL_BUILD_PATH@" KBUILD_VERBOSE=$(MY_KBUILD_VERBOSE) M="$(abs_top_builddir)/aalresourcemgr_server/kbuild" aalkernelinc="$(abs_top_srcdir)/include" modules && \
if [ -f "$(abs_top_builddir)/aalresourcemgr_server/kbuild/@AALRMS_DRV_NAME@.o" ]; then touch "$(abs_top_builddir)/aalresourcemgr_server/kbuild/@AALRMS_DRV_NAME@.o" ; fi && \
if [ -f "$(abs_top_builddir)/aalresourcemgr_server/kbuild/@AALRMS_DRV_NAME@.mod.o" ]; then touch "$(abs_top_builddir)/aalresourcemgr_server/kbuild/@AALRMS_DRV_NAME@.mod.o" ; fi && \
if [ -f "$(abs_top_builddir)/aalresourcemgr_server/kbuild/Module.symvers" ]; then touch "$(abs_top_builddir)/aalresourcemgr_server/kbuild/Module.symvers" ; fi && \
if [ -f "$(abs_top_builddir)/aalresourcemgr_server/kbuild/@AALRMS_DRV_NAME@.ko" ]; then touch "$(abs_top_builddir)/aalresourcemgr_server/kbuild/@AALRMS_DRV_NAME@.ko" ; fi

clean-local:
	@$(MAKE) -C "@AAL_OS_KERNEL_BUILD_PATH@" KBUILD_VERBOSE=$(MY_KBUILD_VERBOSE) M="$(abs_top_builddir)/aalresourcemgr_server/kbuild" clean
