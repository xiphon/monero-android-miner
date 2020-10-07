// BSD 3-Clause License
//
// Copyright (c) 2020, xiphon <xiphon@protonmail.com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <stdexcept>
#include <vector>

#include <randomx.h>

class Vm
{
public:
  Vm(const std::vector<uint8_t> &seedHash)
  {
    randomx_flags flags = randomx_get_flags();

    m_cache = randomx_alloc_cache(flags);
    if (m_cache == nullptr)
    {
      throw std::runtime_error("failed to allocate RandomX cache");
    }
    randomx_init_cache(m_cache, &seedHash[0], seedHash.size());

    m_machine = randomx_create_vm(flags, m_cache, NULL);
    if (m_machine == nullptr)
    {
      throw std::runtime_error("failed to create RandomX vm");
    }
  }

  ~Vm()
  {
    randomx_destroy_vm(m_machine);
    randomx_release_cache(m_cache);
  }

  void setCache(const std::vector<uint8_t> &seedHash)
  {
    randomx_init_cache(m_cache, &seedHash[0], seedHash.size());
    randomx_vm_set_cache(m_machine, m_cache);
  }

  void hash(const std::vector<uint8_t> &blob, std::array<uint8_t, RANDOMX_HASH_SIZE> *result)
  {
    randomx_calculate_hash(m_machine, &blob[0], blob.size(), &(*result)[0]);
  }

private:
  randomx_cache *m_cache;
  randomx_vm *m_machine;
};
