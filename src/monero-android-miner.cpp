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

#include <algorithm>
#include <array>
#include <memory>
#include <mutex>
#include <numeric>
#include <vector>

#include <jni.h>

#include "hasher.h"
#include "job.h"
#include "utils.h"

std::mutex mutex;
std::vector<std::unique_ptr<Hasher>> hashers;
double cpuLoadModifier = 0.5;

extern "C"
{
  JNIEXPORT jboolean JNICALL Java_com_wat_basicidletest_MoneroMining_miningStart(
    JNIEnv *env,
    jobject,
    jstring id,
    jbyteArray blob,
    jbyteArray seedHash,
    jlong height,
    jbyteArray target)
  {
    if (height < std::numeric_limits<size_t>::min())
    {
      return false;
    }

    const std::vector<uint8_t> targetBytes = jbyteArrayToVector(env, target);
    if (targetBytes.size() != Target::Size)
    {
      return false;
    }
    std::array<uint8_t, Target::Size> targetArray;
    std::copy(targetBytes.cbegin(), targetBytes.cbegin() + targetArray.size(), targetArray.begin());

    const std::vector<uint8_t> blobBytes = jbyteArrayToVector(env, blob);
    if (!Job::validateBlob(blobBytes))
    {
      return false;
    }

    const std::vector<uint8_t> seedHashBytes = jbyteArrayToVector(env, seedHash);
    if (!Job::validateSeedHash(seedHashBytes))
    {
      return false;
    }

    {
      std::lock_guard<std::mutex> lock(mutex);

      if (hashers.empty())
      {
        const size_t cpuThreads = std::max(std::thread::hardware_concurrency() / Hasher::MaxCpuCoresDivisor, 1u);
        for (size_t index = 0; index < cpuThreads; ++index)
        {
          hashers.emplace_back(new Hasher(index, cpuThreads, cpuLoadModifier));
        }
      }

      for (auto &hasher : hashers)
      {
        hasher->setJob(
          Job(jstringTostring(env, id), blobBytes, seedHashBytes, static_cast<size_t>(height), targetArray));
      }
    }

    return true;
  }

  JNIEXPORT void JNICALL Java_com_wat_basicidletest_MoneroMining_miningStop(JNIEnv *, jobject)
  {
    std::lock_guard<std::mutex> lock(mutex);

    hashers.clear();
  }

  JNIEXPORT void JNICALL Java_com_wat_basicidletest_MoneroMining_adjustCpuLoad(JNIEnv *, jobject, jdouble modifier)
  {
    std::lock_guard<std::mutex> lock(mutex);

    cpuLoadModifier = modifier;
    for (auto &hasher : hashers)
    {
      hasher->setModifier(cpuLoadModifier);
    }
  }

  JNIEXPORT jdouble JNICALL Java_com_wat_basicidletest_MoneroMining_hashrate(JNIEnv *, jobject)
  {
    std::lock_guard<std::mutex> lock(mutex);

    return std::accumulate(
      hashers.begin(),
      hashers.end(),
      0.0,
      [](const double total, std::unique_ptr<Hasher> &hasher) {
        return total + hasher->hashrate();
      });
  }
}
