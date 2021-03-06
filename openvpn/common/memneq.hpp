//    OpenVPN -- An application to securely tunnel IP networks
//               over a single port, with support for SSL/TLS-based
//               session authentication and key exchange,
//               packet encryption, packet authentication, and
//               packet compression.
//
//    Copyright (C) 2012-2017 OpenVPN Technologies, Inc.
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License Version 3
//    as published by the Free Software Foundation.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program in the COPYING file.
//    If not, see <http://www.gnu.org/licenses/>.

#ifndef OPENVPN_COMMON_MEMNEQ_H
#define OPENVPN_COMMON_MEMNEQ_H

#include <openvpn/common/arch.hpp>
#include <openvpn/common/size.hpp>

// Does this architecture allow efficient unaligned access?

#if defined(OPENVPN_ARCH_x86_64) || defined(OPENVPN_ARCH_i386)
#define OPENVPN_HAVE_EFFICIENT_UNALIGNED_ACCESS
#endif

// Define a portable compiler memory access fence (from Boost).

#if defined(__INTEL_COMPILER)

#define OPENVPN_COMPILER_FENCE __memory_barrier();

#elif defined( _MSC_VER ) && _MSC_VER >= 1310

extern "C" void _ReadWriteBarrier();
#pragma intrinsic( _ReadWriteBarrier )

#define OPENVPN_COMPILER_FENCE _ReadWriteBarrier();

#elif defined(__GNUC__)

#define OPENVPN_COMPILER_FENCE __asm__ __volatile__( "" : : : "memory" );

#else

#error need memory fence definition for this compiler

#endif

// C++ doesn't allow increment of void *

#define OPENVPN_INCR_VOID_PTR(var, incr) (var) = static_cast<const unsigned char*>(var) + (incr)

namespace openvpn {
  namespace crypto {

#ifdef OPENVPN_HAVE_EFFICIENT_UNALIGNED_ACCESS
    enum { memneq_unaligned_ok = 1 };
    typedef size_t memneq_t;
#else
    enum { memneq_unaligned_ok = 0 };
    typedef unsigned int memneq_t;
#endif

    // Is value of type T aligned on A boundary?
    // NOTE: requires that sizeof(A) is a power of 2
    template <typename T, typename A>
    inline bool is_aligned(const T value)
    {
      return (size_t(value) & (sizeof(A)-1)) == 0;
    }

    // Returns true if we are allowed to dereference a and b that
    // point to objects of type memneq_t.
    inline bool memneq_deref_ok(const void *a, const void *b)
    {
      return memneq_unaligned_ok || (is_aligned<const void *, memneq_t>(a)|is_aligned<const void *, memneq_t>(b));
    }

    // Constant-time memory equality method.  Can be used in
    // security-sensitive contexts to inhibit timing attacks.
    inline bool memneq(const void *a, const void *b, size_t size)
    {
      memneq_t neq = 0;

      if (memneq_deref_ok(a, b))
	{
	  while (size >= sizeof(memneq_t))
	    {
	      neq |= *(memneq_t *)a ^ *(memneq_t *)b;
	      OPENVPN_INCR_VOID_PTR(a, sizeof(memneq_t));
	      OPENVPN_INCR_VOID_PTR(b, sizeof(memneq_t));
	      size -= sizeof(memneq_t);
	    }
	}
      while (size > 0)
	{
	  neq |= *(unsigned char *)a ^ *(unsigned char *)b;
	  OPENVPN_INCR_VOID_PTR(a, 1);
	  OPENVPN_INCR_VOID_PTR(b, 1);
	  size -= 1;
	}
      OPENVPN_COMPILER_FENCE
      return bool(neq);
    }
  }
}

#endif
