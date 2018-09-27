/*-------------------------------------------------------------------------
This source file is a part of Placid

For the latest info, see http://www.marrin.org/

Copyright (c) 2018, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.
    
    - Redistributions in binary form must reproduce the above copyright 
    notice, this list of conditions and the following disclaimer in the 
    documentation and/or other materials provided with the distribution.
    
    - Neither the name of the <ORGANIZATION> nor the names of its 
    contributors may be used to endorse or promote products derived from 
    this software without specific prior written permission.
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#include <cstddef>
#include <unistd.h>
#include <cstdlib>

extern "C" {
	
// Needed by dlmalloc to get page size
long sysconf(int name)
{
	return (name == _SC_PAGE_SIZE) ? 1024 : 0;
}

// Needed by dlmalloc (need to implement this properly eventually)
void abort()
{
	while (1) ;
}

void __aeabi_idiv0()
{
	abort();
}

int __errno = 0;

void *mmap(void *addr, size_t length, int prot, int flags, int fd, size_t offset)
{
	return nullptr;
}

int munmap(void *addr, size_t length)
{
	return 0;
}

}

void *operator new(size_t size)
{
	return malloc(size);
}

void *operator new[] (size_t size)
{
	return malloc(size);
}

void operator delete(void *p) noexcept
{
	free(p);
}

void operator delete [ ](void *p) noexcept
{
	free(p);
}

void operator delete(void *p, size_t size) noexcept
{
	free(p);
}

void operator delete [ ](void *p, size_t size) noexcept
{
	free(p);
}
