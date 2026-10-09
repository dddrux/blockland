// stub minimo de <jni.h> so para checar a SINTAXE do ramo Android do Monitor (-DBL_MON_FORCE_ANDROID); nao serve para executar
#pragma once
#include <cstdint>
#include <cstdarg>
typedef int32_t jint; typedef float jfloat; typedef uint8_t jboolean; typedef void* jobject; typedef void* jclass; typedef void* jmethodID; typedef void* jstring; typedef void* jintArray;
struct JNIEnv { jclass GetObjectClass(jobject); jmethodID GetMethodID(jclass, const char*, const char*); void CallVoidMethodV(jobject, jmethodID, va_list); bool ExceptionCheck(); void ExceptionClear(); void DeleteLocalRef(void*); jstring NewStringUTF(const char*); int GetArrayLength(jintArray); void GetIntArrayRegion(jintArray, int, int, jint*); };
#define JNIEXPORT
#define JNICALL
void* SDL_AndroidGetJNIEnv(); void* SDL_AndroidGetActivity();
