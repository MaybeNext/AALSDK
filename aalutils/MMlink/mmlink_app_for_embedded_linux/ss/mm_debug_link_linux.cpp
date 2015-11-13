/*
 * Copyright (c) 2014, Altera Corporation.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Altera Corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <fcntl.h>
#include "mm-debug-link.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>

#include "mm_debug_link_linux.h"
#include "printf.h"

#define DRIVER_PATH "/dev/mm_debug_link"
#define B2P_EOP 0x7B

#define BASE_ADDR 4096

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

#define MM_DEBUG_LINK_DATA_WRITE        0x00
#define MM_DEBUG_LINK_WRITE_CAPACITY    0x04
#define MM_DEBUG_LINK_DATA_READ         0x08
#define MM_DEBUG_LINK_READ_CAPACITY     0x0C
#define MM_DEBUG_LINK_FIFO_WRITE_COUNT  0x20
#define MM_DEBUG_LINK_FIFO_READ_COUNT   0x40
#define MM_DEBUG_LINK_ID_ROM            0x60
#define MM_DEBUG_LINK_SIGNATURE         0x70
#define MM_DEBUG_LINK_VERSION           0x74
#define MM_DEBUG_LINK_DEBUG_RESET       0x78
#define MM_DEBUG_LINK_MGMT_INTF         0x7C


/*
* The value to expect at offset MM_DEBUG_LINK_SIGNATURE, aka "SysC".
*/
#define EXPECT_SIGNATURE 0x53797343

/*
* The maximum version this driver supports.
*/
#define MAX_SUPPORTED_VERSION 1


mm_debug_link_interface *get_mm_debug_link(void)
{
  return new mm_debug_link_linux();
}

int mm_debug_link_linux::open(char* filename)
{
	  struct stat sts;
	  unsigned int sign, version;
	  m_fd = -1;

	  if ((stat(filename, &sts)) == -1)
	  {
	    printf("[MM Link Task] Failed to open %s. MM Debug Link Driver may not be loaded.\n", filename);
	    exit(EXIT_FAILURE);
	  }

	  //m_fd = ::open(drvPath, O_RDWR);
	  m_fd = ::open(filename, O_RDWR | O_SYNC);

	  if (m_fd < 0)
	  {
	    printf("failed to open sys file. fd = %d\n", m_fd);
	    return m_fd;
	  }

	  map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, (BASE_ADDR) & ~MAP_MASK);
	  if(map_base == (void *) -1)
	  {
		printf("[MM Link Task] Failed to Map one page\n");
		return -1;
	  }

	  sign = *(static_cast<unsigned int*>(read_mmr(MM_DEBUG_LINK_SIGNATURE, 'w')));
	  printf( "%s %s(): read signature value %x to hw\n", __FILE__, __FUNCTION__, sign );
	  if ( sign != EXPECT_SIGNATURE)
	  {
		printf("[MM Link Task] Unverified Signature\n");
		return -1;
	  }

	  version = *(static_cast<int*>(read_mmr(MM_DEBUG_LINK_VERSION, 'w')));
	  printf( "%s %s(): read version value %d to hw\n", __FILE__, __FUNCTION__, version );
	  if ( version > MAX_SUPPORTED_VERSION )
	  {
	   printf("[MM Link Task] Unsupported Version\n");
	   return -1;
	  }

	  this->m_write_fifo_capacity = *(static_cast<int*>(read_mmr(MM_DEBUG_LINK_WRITE_CAPACITY, 'w')));
	  printf( "%s %s(): read write fifo capacity value %d to hw\n", __FILE__, __FUNCTION__, this->m_write_fifo_capacity);

	  return 0;
}

void* mm_debug_link_linux::read_mmr(off_t target, int access_type)
{
	  void *virt_addr;
	  void *read_result;
	  /* Map one page */

	  virt_addr = map_base + ((BASE_ADDR + target) & MAP_MASK);
	  switch(access_type) {
	 		case 'b':
	 			read_result = (void *)((unsigned char *) virt_addr);
	 			break;
	 		case 'h':
	 			read_result = (void *)((unsigned short *) virt_addr);
	 			break;
	 		case 'w':
	 			read_result = (void *)((unsigned int *) virt_addr);
	 			break;
	 		default:
	 			fprintf(stderr, "Illegal data type '%c'.\n", access_type);
	 			exit(2);
	 	}
	  return read_result;
}

void mm_debug_link_linux::write_mmr(off_t target, int access_type, unsigned int write_val)
{
	  void *virt_addr;
	  void *read_result;
	  /* Map one page */

	  virt_addr = map_base + ((BASE_ADDR + target) & MAP_MASK);
	  switch(access_type) {
	 		case 'b':
	 			*((unsigned char *) virt_addr) = write_val;
	 			break;
	 		case 'h':
	 			*((unsigned short *) virt_addr) = write_val;
	 			break;
	 		case 'w':
	 			*((unsigned long *) virt_addr) = write_val;
	 			break;
	 		default:
	 			fprintf(stderr, "Illegal data type '%c'.\n", access_type);
	 			exit(2);
	 	}
}

ssize_t mm_debug_link_linux::read()
{
	unsigned char  num_bytes;

	  num_bytes = *(static_cast<unsigned char *>(read_mmr(MM_DEBUG_LINK_FIFO_READ_COUNT, 'b' )));
	  if (num_bytes > 0 )
	  {
	    if ( num_bytes > (mm_debug_link_linux::BUFSIZE - m_buf_end) )
	    {
	      num_bytes = mm_debug_link_linux::BUFSIZE - m_buf_end;
	    }
	    printf( "%s %s(): read %d bytes\n", __FILE__, __FUNCTION__, num_bytes );
	    for ( unsigned char i = 0; i < num_bytes; ++i )
	    {
	    	*(this->m_buf + this->m_buf_end + i)= *(static_cast<unsigned char*>(read_mmr( MM_DEBUG_LINK_DATA_READ, 'b')));
	    }

	      unsigned int x;
	      for ( unsigned char i = 0; i < num_bytes; ++i )
	      {
	        x = this->m_buf[this->m_buf_end + i];
	        printf( "%02X ", x );
	      }
	      printf( "\n" );

	      this->m_buf_end += num_bytes;
	  }
	  else
	  {
	      //printf( "%s %s(): error read hw read buffer level\n", __FILE__, __FUNCTION__ );
	      num_bytes = 0;
	  }

	  return num_bytes;
}

ssize_t mm_debug_link_linux::write(const void *buf, size_t count)
{
	unsigned char  num_bytes;
    unsigned int x;
	  num_bytes = *(static_cast<unsigned char*>(read_mmr(MM_DEBUG_LINK_FIFO_WRITE_COUNT, 'b' )));
	  if ( num_bytes < this->m_write_fifo_capacity )
	  {
	    num_bytes = this->m_write_fifo_capacity - num_bytes;
	    if ( count < num_bytes )
	    {
	      num_bytes = count;
	    }
	    for ( size_t i = 0; i < num_bytes; ++i )
	    {
	      write_mmr( MM_DEBUG_LINK_DATA_WRITE, 'b', *((unsigned char *)buf + i));
	    }

	      printf( "%s %s(): wrote %d bytes\n", __FILE__, __FUNCTION__, num_bytes );

	      for ( int i = 0; i < num_bytes; ++i )
	      {
	        x = *((unsigned char *)buf + i);
	        printf( "%02X ", x );
	      }
	      printf( "\n" );
	  }
	  else
	  {
		printf( "%s %s(): error write hw write buffer level\n", __FILE__, __FUNCTION__ );
	    num_bytes = 0;
	  }

	  return num_bytes;
}

void mm_debug_link_linux::close(void)
{
  printf("mm_debug_link_linux::close\n");
  if(munmap(map_base, MAP_SIZE) == -1) printf("Unmap error\n");

  if (m_fd != -1)
    ::close(m_fd);
  m_fd = -1;
}

void mm_debug_link_linux::write_ident(int val)
{
	write_mmr(MM_DEBUG_LINK_ID_ROM, 'b', val);
	printf( "%s %s(): write mixer value %d to hw\n", __FILE__, __FUNCTION__, val );
}

void mm_debug_link_linux::reset(bool val)
{
	unsigned int reset_val = val ? 1 : 0;
	write_mmr(MM_DEBUG_LINK_DEBUG_RESET, 'w', val);
	printf( "%s %s(): write reset value %d to hw\n", __FILE__, __FUNCTION__, reset_val );
}

void mm_debug_link_linux::ident(int id[4])
{
	 for ( int i = 0; i < 4; i++ )
	  {
		 id[i] = *(static_cast<int*>(read_mmr(MM_DEBUG_LINK_ID_ROM + i * 4, 'w')));
	  }
}

void mm_debug_link_linux::enable(int channel, bool state)
{
  int encoded_cmd = (channel << 8) | (state ? 1 : 0);
  write_mmr(MM_DEBUG_LINK_MGMT_INTF, 'w', encoded_cmd);
  printf( "%s %s(): enable channel %d to hw\n", __FILE__, __FUNCTION__, encoded_cmd );

}

bool mm_debug_link_linux::flush_request(void)
{
  bool should_flush = false;
  if (m_buf_end == BUFSIZE)
    // Full buffer? Send.
    should_flush = true;
  else if (memchr(m_buf, B2P_EOP, m_buf_end - 1))
    // Buffer contains eop? Send.
    // If the eop character occurs in the very last buffer byte, there's no packet here - 
    // we need at least one more byte.
    // Interesting corner case: it's not strictly true that one more byte after EOP indicates
    // the end of a packet - that byte after EOP might be the escape character. In this case,
    // we flush even though it's not necessarily a complete packet. This probably has negligible
    // impact on performance.
    should_flush = true;

  if ( m_buf_end > 0 )
   {
     should_flush = true;
   }
  return should_flush;
}

