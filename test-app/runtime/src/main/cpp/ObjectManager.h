#ifndef OBJECTMANAGER_H_
#define OBJECTMANAGER_H_

#include <chrono>

#include "v8.h"
#include "JEnv.h"
#include "JniLocalRef.h"
#include "ArgsWrapper.h"
#include "LRUCache.h"
#include <map>
#include <set>
#include <stack>
#include <vector>
#include <string>

#include <jni.h>

#include <cppgc/garbage-collected.h>
#include <cppgc/persistent.h>
#include <v8-cppgc.h>

#include "GCUtil.h"
#include "JavaObjectMap.h"

namespace tns {
class JSInstanceInfo;

class ObjectManager {
    public:
        ObjectManager(v8::Isolate* isolate, jobject javaRuntimeObject);

        JniLocalRef GetJavaObjectByJsObject(const v8::Local<v8::Object>& object);

        jint GetOrCreateObjectId(jobject object);

        v8::Local<v8::Object> GetJsObjectByJavaObject(jint javaObjectID);
        void DropJSObjectByJavaObject(jint javaObjectID);

        v8::Local<v8::Object> CreateJSWrapper(jint javaObjectID, const std::string& typeName);

        v8::Local<v8::Object> CreateJSWrapper(jint javaObjectID, const std::string& typeName, jobject instance);

        void Link(const v8::Local<v8::Object>& object, jint javaObjectID);

        void ReleaseNativeCounterpart(v8::Local<v8::Object>& object);

        bool CloneLink(const v8::Local<v8::Object>& src, const v8::Local<v8::Object>& dest);

        jint GenerateNewObjectID();

        v8::Local<v8::Object> GetEmptyObject(v8::Isolate* isolate);

        void MakeJavaInstanceWeak(jint javaObjectID);
        bool IsJavaInstanceAlive(jint javaObjectID);

        enum class MetadataNodeKeys {
            CallSuper = kMinGarbageCollectedEmbedderFields,
            END
        };

        void LogGCStats();

    private:
        JSInstanceInfo* GetJSInstanceInfo(const v8::Local<v8::Object>& object);

        JSInstanceInfo* GetJSInstanceInfoFromRuntimeObject(const v8::Local<v8::Object>& object);

        v8::Local<v8::Object> CreateJSWrapperHelper(jint javaObjectID, const std::string& typeName, jclass clazz);

        jweak GetJavaObjectByID(jint javaObjectID);

        jobject GetJavaObjectByIDImpl(jint javaObjectID);

        static jweak NewWeakGlobalRefCallback(const int& javaObjectID, void* state);

        static void DeleteWeakGlobalRefCallback(const jweak& object, void* state);

        static bool IsJsRuntimeObject(const v8::Local<v8::Object>& object);

        static void OnGCStart(v8::Isolate* isolate, v8::GCType type, v8::GCCallbackFlags flags, void* data);
        static void OnGCFinish(v8::Isolate* isolate, v8::GCType type, v8::GCCallbackFlags flags, void* data);

        jobject m_javaRuntimeObject;

        v8::Isolate* m_isolate;

        cppgc::Persistent<JavaObjectMap> m_idToObject;

        LRUCache<jint, jweak> m_cache;

        volatile jint m_currentObjectId;

        jmethodID GET_JAVAOBJECT_BY_ID_METHOD_ID;

        jmethodID GET_OR_CREATE_JAVA_OBJECT_ID_METHOD_ID;

        jmethodID IS_INSTANCE_ALIVE_METHOD_ID;
        jmethodID MAKE_INSTANCE_WEAK_METHOD_ID;

        jmethodID RELEASE_NATIVE_INSTANCE_METHOD_ID;

        v8::Persistent<v8::ObjectTemplate> m_wrapperObjectTemplate;

        std::chrono::steady_clock::time_point m_gcStartTime;
        size_t m_gcCounts[5] = {0, 0, 0, 0, 0};
        std::chrono::steady_clock::duration m_gcDurations[5] = {{}, {}, {}, {}, {}};
};
}

#endif /* OBJECTMANAGER_H_ */
