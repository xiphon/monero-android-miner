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
#include <string>
#include <vector>

#include <randomx.h>

#include "target.h"

class Job
{
public:
  static constexpr const size_t NonceOffset = 39;
  typedef uint32_t Nonce;

  Job &operator=(const Job &other)
  {
    m_id = other.m_id;
    m_blob = other.m_blob;
    m_seedHash = other.m_seedHash;
    m_height = other.m_height;
    m_target = other.m_target;

    return *this;
  }

  Job(std::string id, std::vector<uint8_t> blob, std::vector<uint8_t> seedHash, size_t height, Target target)
    : m_id(std::move(id))
    , m_blob(std::move(blob))
    , m_seedHash(std::move(seedHash))
    , m_height(height)
    , m_target(std::move(target))
  {
    if (!validateBlob(m_blob))
    {
      throw std::runtime_error("invalid blob length");
    }
    if (!validateSeedHash(m_seedHash))
    {
      throw std::runtime_error("invalid seed hash length");
    }
  }

  static bool validateBlob(const std::vector<uint8_t> &blob)
  {
    return blob.size() >= NonceOffset + sizeof(Nonce);
  }

  static bool validateSeedHash(const std::vector<uint8_t> &seedHash)
  {
    return seedHash.size() == RANDOMX_HASH_SIZE;
  }

  const std::vector<uint8_t> &blob() const
  {
    return m_blob;
  }

  const std::vector<uint8_t> &seedHash() const
  {
    return m_seedHash;
  }

  void nonceSet(Nonce nonce)
  {
    *reinterpret_cast<Nonce *>(&m_blob[NonceOffset]) = nonce;
  }

  void nonceAdd(Nonce value)
  {
    *reinterpret_cast<Nonce *>(&m_blob[NonceOffset]) += value;
  }

  Nonce nonce() const
  {
    return *reinterpret_cast<const Nonce *>(&m_blob[NonceOffset]);
  }

  bool seedEqual(const Job &other) const
  {
    return std::equal(m_seedHash.begin(), m_seedHash.end(), other.m_seedHash.begin(), other.m_seedHash.end());
  }

  const Target &target() const
  {
    return m_target;
  }

  std::string id() const
  {
    return m_id;
  }

private:
  std::string m_id;
  std::vector<uint8_t> m_blob;
  std::vector<uint8_t> m_seedHash;
  size_t m_height;
  Target m_target;
};
