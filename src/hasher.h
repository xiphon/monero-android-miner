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

#include <memory>
#include <mutex>
#include <thread>

#include <jni.h>

#include "callback.h"
#include "hashrate.h"
#include "job.h"
#include "regulator.h"
#include "utils.h"
#include "vm.h"

class Hasher : public Regulator, public Hashrate
{
public:
  Hasher(size_t id, size_t concurrency, double modifier)
    : m_id(id)
    , m_concurrency(concurrency)
    , Regulator(modifier)
  {
  }

  ~Hasher()
  {
    m_canRun.clear();

    try
    {
      m_thread.join();
    }
    catch (...)
    {
    }
  }

  void setJob(Job job)
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_job.reset(new Job(job));
    m_canRun.test_and_set();
    m_updated.clear();

    if (!m_thread.joinable())
    {
      m_thread = std::thread([this, job]() {
        JNIEnv *env;
        JavaVMAttachArgs lJavaVMAttachArgs;
        lJavaVMAttachArgs.version = JNI_VERSION_1_6;
        lJavaVMAttachArgs.name = "HasherThread";
        lJavaVMAttachArgs.group = NULL;
        if (javaVm->AttachCurrentThread(&env, &lJavaVMAttachArgs) == JNI_ERR)
        {
          throw std::runtime_error("AttachCurrentThread failed");
        }

        try
        {
          thread(CallbackVoidStringStringString(env, className, methodName), std::move(job));
        }
        catch (...)
        {
        }

        javaVm->DetachCurrentThread();
      });
    }
  }

private:
  void thread(const CallbackVoidStringStringString &callback, Job job)
  {
    std::array<uint8_t, RANDOMX_HASH_SIZE> result;

    m_vm.reset(new Vm(job.seedHash()));

    Hashrate::reset();
    while (m_canRun.test_and_set())
    {
      if (!m_updated.test_and_set())
      {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_job)
        {
          continue;
        }
        Job newJob = *m_job;
        if (!job.seedEqual(newJob))
        {
          m_vm->setCache(newJob.seedHash());
        }
        job = Job(newJob);
        job.nonceSet(m_id);
      }

      m_vm->hash(job.blob(), &result);

      if (job.target() > result)
      {
        Job::Nonce nonce = job.nonce();
        callback.invoke(
          job.id(),
          bufferToHex(&result[0], result.size()),
          bufferToHex(reinterpret_cast<uint8_t *>(&nonce), sizeof(nonce)));
      }

      job.nonceAdd(m_concurrency);
      Hashrate::tick();
      Regulator::tick();
    }
  }

private:
  std::unique_ptr<Vm> m_vm;
  const size_t m_id;
  const size_t m_concurrency;

  std::atomic_flag m_updated;
  std::atomic_flag m_canRun;
  std::mutex m_mutex;
  std::unique_ptr<Job> m_job;

  std::thread m_thread;
};
