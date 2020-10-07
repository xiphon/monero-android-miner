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

#include <jni.h>

JavaVM *javaVm = nullptr;
jobject classLoader;
jmethodID findClassMethod;

constexpr const char className[] = "com/wat/basicidletest/MoneroMining";
constexpr const char methodName[] = "miningCallback";

extern "C"
{
  // Átila Neves https://stackoverflow.com/a/16302771
  JNIEnv *getEnv()
  {
    JNIEnv *env;
    int status = javaVm->GetEnv((void **)&env, JNI_VERSION_1_6);
    if (status < 0)
    {
      status = javaVm->AttachCurrentThread(&env, NULL);
      if (status < 0)
      {
        return nullptr;
      }
    }
    return env;
  }

  JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *pjvm, void *reserved)
  {
    javaVm = pjvm; // cache the JavaVM pointer
    auto env = getEnv();
    // replace with one of your classes in the line below
    auto randomClass = env->FindClass(className);
    jclass classClass = env->GetObjectClass(randomClass);
    auto classLoaderClass = env->FindClass("java/lang/ClassLoader");
    auto getClassLoaderMethod = env->GetMethodID(classClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    classLoader = env->NewGlobalRef(env->CallObjectMethod(randomClass, getClassLoaderMethod));
    findClassMethod = env->GetMethodID(classLoaderClass, "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");

    return JNI_VERSION_1_6;
  }

  JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved)
  {
    getEnv()->DeleteGlobalRef(classLoader);
  }

  jclass findClass(const char *name)
  {
    return static_cast<jclass>(getEnv()->CallObjectMethod(classLoader, findClassMethod, getEnv()->NewStringUTF(name)));
  }
}

class CallbackVoidStringStringString
{
public:
  CallbackVoidStringStringString(JNIEnv *env, const char *className, const char *methodName)
    : m_env(env)
  {
    constexpr const char signature[] = "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V";

    jclass classObject = findClass(className);
    if (classObject == nullptr)
    {
      throw std::runtime_error("java class not found");
    }
    m_class = static_cast<jclass>(m_env->NewGlobalRef(classObject));
    m_method = env->GetStaticMethodID(m_class, methodName, signature);
  }

  ~CallbackVoidStringStringString()
  {
    m_env->DeleteGlobalRef(m_class);
  }

  void invoke(const std::string &jobId, const std::string &hash, const std::string &nonce) const
  {
    m_env->CallStaticVoidMethod(
      m_class,
      m_method,
      m_env->NewStringUTF(jobId.c_str()),
      m_env->NewStringUTF(hash.c_str()),
      m_env->NewStringUTF(nonce.c_str()));
  }

private:
  JNIEnv *m_env;
  jclass m_class;
  jmethodID m_method;
};
