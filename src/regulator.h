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

#include <atomic>
#include <chrono>
#include <thread>

class Regulator
{
  static constexpr const size_t TicksToMeasure = 5;

public:
  static constexpr const unsigned MaxCpuCoresDivisor = 2;

  Regulator(double modifier)
    : m_ticks(0)
  {
    setModifier(modifier);
  }

  void setModifier(double modifier)
  {
    m_modifier = std::min(1.0 / MaxCpuCoresDivisor, modifier);
  }

  void tick()
  {
    const size_t tick = m_ticks + 1;
    m_ticks = tick % TicksToMeasure;

    if (tick == 1)
    {
      m_first = std::chrono::steady_clock::now();
    }
    else if (tick == TicksToMeasure)
    {
      const auto current = std::chrono::steady_clock::now();
      const double elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(current - m_first).count();
      const size_t sleepMs = static_cast<size_t>(elapsedMs - elapsedMs * m_modifier * MaxCpuCoresDivisor);
      if (sleepMs > 0)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
      }
    }
  }

private:
  std::atomic<size_t> m_ticks;
  std::atomic<double> m_modifier;
  std::chrono::steady_clock::time_point m_first;
};
