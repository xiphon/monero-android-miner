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

#include <iomanip>
#include <sstream>
#include <string>

#include <jni.h>

std::string bufferToHex(const uint8_t *buffer, size_t size)
{
  std::stringstream ss;
  for (size_t index = 0; index < size; ++index)
  {
    ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(buffer[index]);
  }
  return ss.str();
}

std::string bufferToHex(const std::vector<uint8_t> &buffer)
{
  return bufferToHex(&buffer[0], buffer.size());
}

// by Mr Jerry https://stackoverflow.com/a/41820336
std::string jstringTostring(JNIEnv *env, jstring jStr)
{
  if (!jStr)
    return "";

  const jclass stringClass = env->GetObjectClass(jStr);
  const jmethodID getBytes = env->GetMethodID(stringClass, "getBytes", "(Ljava/lang/String;)[B");
  const jbyteArray stringJbytes = (jbyteArray)env->CallObjectMethod(jStr, getBytes, env->NewStringUTF("UTF-8"));

  size_t length = (size_t)env->GetArrayLength(stringJbytes);
  jbyte *pBytes = env->GetByteArrayElements(stringJbytes, NULL);

  std::string ret = std::string((char *)pBytes, length);
  env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);

  env->DeleteLocalRef(stringJbytes);
  env->DeleteLocalRef(stringClass);
  return ret;
}

// by Mustafa Kemal https://stackoverflow.com/a/25804198
std::vector<uint8_t> jbyteArrayToVector(JNIEnv *env, jbyteArray jbIn)
{
  std::vector<uint8_t> result;

  size_t size = env->GetArrayLength(jbIn);
  uint8_t *bufferIn = static_cast<uint8_t *>(env->GetPrimitiveArrayCritical(jbIn, nullptr));
  if (bufferIn != nullptr)
  {
    result.assign(&bufferIn[0], &bufferIn[size]);
    env->ReleasePrimitiveArrayCritical(jbIn, reinterpret_cast<void *>(bufferIn), 0);
  }
  return result;
}
