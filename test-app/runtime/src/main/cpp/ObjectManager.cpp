#include <chrono>

#include "ObjectManager.h"
#include "NativeScriptAssert.h"
#include "MetadataNode.h"
#include "ArgConverter.h"
#include "Util.h"
#include "V8GlobalHelpers.h"
#include "V8StringConstants.h"
#include "NativeScriptException.h"
#include "Runtime.h"
#include "include/v8.h"
#include <algorithm>
#include <sstream>

#include "GCUtil.h"
#include "JSInstanceInfo.h"
#include "ManualInstrumentation.h"

using namespace v8;
using namespace std;
using namespace tns;

ObjectManager::ObjectManager(v8::Isolate* isolate, jobject javaRuntimeObject) :
        m_isolate(isolate),
        m_javaRuntimeObject(javaRuntimeObject),
        m_currentObjectId(0),
        m_idToObject(MakeGarbageCollected<JavaObjectMap>(isolate)),
        m_cache(NewWeakGlobalRefCallback, DeleteWeakGlobalRefCallback, 1000, this) {

    JEnv env;
    auto runtimeClass = env.FindClass("com/tns/Runtime");
    assert(runtimeClass != nullptr);

    GET_JAVAOBJECT_BY_ID_METHOD_ID = env.GetMethodID(runtimeClass, "getJavaObjectByID",
                                                       "(I)Ljava/lang/Object;");
    assert(GET_JAVAOBJECT_BY_ID_METHOD_ID != nullptr);

    GET_OR_CREATE_JAVA_OBJECT_ID_METHOD_ID = env.GetMethodID(runtimeClass,
                                                               "getOrCreateJavaObjectID",
                                                               "(Ljava/lang/Object;)I");
    assert(GET_OR_CREATE_JAVA_OBJECT_ID_METHOD_ID != nullptr);

    IS_INSTANCE_ALIVE_METHOD_ID = env.GetMethodID(runtimeClass, "isInstanceAlive", "(I)Z");
    MAKE_INSTANCE_WEAK_METHOD_ID = env.GetMethodID(runtimeClass,"makeInstanceWeak", "(I)V");
    assert(IS_INSTANCE_ALIVE_METHOD_ID != nullptr);
    assert(MAKE_INSTANCE_WEAK_METHOD_ID != nullptr);

    RELEASE_NATIVE_INSTANCE_METHOD_ID = env.GetMethodID(runtimeClass, "releaseNativeCounterpart",
                                                          "(I)V");
    assert(RELEASE_NATIVE_INSTANCE_METHOD_ID != nullptr);

    jclass javaLangClass = env.FindClass("java/lang/Class");
    assert(javaLangClass != nullptr);

    Local<ObjectTemplate> tmpl = ObjectTemplate::New(m_isolate);
    tmpl->SetInternalFieldCount(static_cast<int>(MetadataNodeKeys::END));
    m_wrapperObjectTemplate.Reset(m_isolate, tmpl);

    m_isolate->AddGCPrologueCallback(&ObjectManager::OnGCStart, this, kGCTypeAll);
    m_isolate->AddGCEpilogueCallback(&ObjectManager::OnGCFinish, this, kGCTypeAll);
}


JniLocalRef ObjectManager::GetJavaObjectByJsObject(const Local<Object> &object) {
    JSInstanceInfo *jsInstanceInfo = GetJSInstanceInfo(object);

    if (jsInstanceInfo != nullptr) {
        JniLocalRef javaObject(GetJavaObjectByID(jsInstanceInfo->JavaObjectID()), true);
        return javaObject;
    }

    return JniLocalRef();
}

JSInstanceInfo* ObjectManager::GetJSInstanceInfo(const Local<Object> &object) {
    if (IsJsRuntimeObject(object)) {
        return GetJSInstanceInfoFromRuntimeObject(object);
    }
    return nullptr;
}

JSInstanceInfo* ObjectManager::GetJSInstanceInfoFromRuntimeObject(const Local<Object> &object) {
    HandleScope handleScope(m_isolate);

    auto* jsInfo = ExtractWrapper<JSInstanceInfo>(object);
    if (!jsInfo) {
        //Typescript object layout has an object instance as child of the actual registered instance. checking for that
        auto prototypeObject = object->GetPrototype().As<Object>();

        if (!prototypeObject.IsEmpty() && prototypeObject->IsObject()) {
            DEBUG_WRITE("GetJSInstanceInfo: need to check prototype :%d",
                        prototypeObject->GetIdentityHash());
            if (IsJsRuntimeObject(prototypeObject)) {
                jsInfo = ExtractWrapper<JSInstanceInfo>(prototypeObject);
            }
        }
    }
    return jsInfo;
}

bool ObjectManager::IsJsRuntimeObject(const v8::Local<v8::Object> &object) {
    int internalFieldCount = object->InternalFieldCount();
    const int count = static_cast<int>(MetadataNodeKeys::END);
    return internalFieldCount == count;
}

jweak ObjectManager::GetJavaObjectByID(jint javaObjectID) {
    jweak obj = m_cache(javaObjectID);

    return obj;
}

jobject ObjectManager::GetJavaObjectByIDImpl(jint javaObjectID) {
    JEnv env;
    jobject object = env.CallObjectMethod(m_javaRuntimeObject, GET_JAVAOBJECT_BY_ID_METHOD_ID,
                                            javaObjectID);
    return object;
}

jint ObjectManager::GetOrCreateObjectId(jobject object) {
    JEnv env;
    jint javaObjectID = env.CallIntMethod(m_javaRuntimeObject,
                                            GET_OR_CREATE_JAVA_OBJECT_ID_METHOD_ID, object);

    return javaObjectID;
}

Local<Object> ObjectManager::GetJsObjectByJavaObject(jint javaObjectID) {
    return m_idToObject->Get(m_isolate, javaObjectID);
}

void ObjectManager::DropJSObjectByJavaObject(jint javaObjectID) {
    m_idToObject->Drop(javaObjectID);
}

Local<Object> ObjectManager::CreateJSWrapper(jint javaObjectID, const string &typeName) {
    return CreateJSWrapperHelper(javaObjectID, typeName, nullptr);
}

Local<Object>
ObjectManager::CreateJSWrapper(jint javaObjectID, const string &typeName, jobject instance) {
    JEnv env;
    JniLocalRef clazz(env.GetObjectClass(instance));

    return CreateJSWrapperHelper(javaObjectID, typeName, clazz);
}

Local<Object>
ObjectManager::CreateJSWrapperHelper(jint javaObjectID, const string &typeName, jclass clazz) {
    auto isolate = m_isolate;

    std::string className{clazz != nullptr ? JEnv{}.GetClassName(clazz) : typeName};

    auto node = MetadataNode::GetOrCreate(className);

    auto jsWrapper = node->CreateJSWrapper(isolate, this);

    if (!jsWrapper.IsEmpty()) {
        Link(jsWrapper, javaObjectID);
    }
    return jsWrapper;
}


/* *
 * Link the JavaScript object and it's java counterpart with an ID
 */
void ObjectManager::Link(const Local<Object> &object, jint javaObjectID) {
    if (!IsJsRuntimeObject(object)) {
        string errMsg("Trying to link invalid 'this' to a Java object");
        throw NativeScriptException(errMsg);
    }

    auto isolate = m_isolate;

    DEBUG_WRITE("Linking js object: %d and java instance id: %d", object->GetIdentityHash(),
                javaObjectID);

    auto* jsInstanceInfo = MakeGarbageCollected<JSInstanceInfo>(isolate, javaObjectID, this);
    AttachGarbageCollectedWrapper(object, jsInstanceInfo);

    m_idToObject->Add(isolate, javaObjectID, object);
}

bool ObjectManager::CloneLink(const Local<Object> &src, const Local<Object> &dest) {
    auto jsInfo = GetJSInstanceInfo(src);

    auto success = jsInfo != nullptr;

    if (success) {
        AttachGarbageCollectedWrapper(dest, jsInfo);  // FIXME clone the jsInfo?
    }

    return success;
}

jint ObjectManager::GenerateNewObjectID() {
    const jint one = 1;
    jint oldValue = __sync_fetch_and_add(&m_currentObjectId, one);

    return oldValue;
}

jweak ObjectManager::NewWeakGlobalRefCallback(const jint &javaObjectID, void *state) {
    auto objManager = reinterpret_cast<ObjectManager *>(state);

    JniLocalRef obj(objManager->GetJavaObjectByIDImpl(javaObjectID));

    JEnv env;
    jweak weakRef = env.NewWeakGlobalRef(obj);

    return weakRef;
}

void ObjectManager::DeleteWeakGlobalRefCallback(const jweak &object, [[maybe_unused]] void *state) {
    JEnv env;
    env.DeleteWeakGlobalRef(object);
}

Local<Object> ObjectManager::GetEmptyObject(Isolate *isolate) {
    auto context = Runtime::GetRuntime(isolate)->GetContext();
    return m_wrapperObjectTemplate.Get(isolate)->NewInstance(context).ToLocalChecked();
}

void ObjectManager::ReleaseNativeCounterpart(v8::Local<v8::Object> &object) {

    if(!object->IsObject()){
        throw NativeScriptException("Argument is not an object!");
    }

    JSInstanceInfo *jsInstanceInfo = GetJSInstanceInfo(object);

    if(jsInstanceInfo == nullptr){
        throw NativeScriptException("Trying to release a non native object!");
    }

    JEnv env;
    env.CallVoidMethod(m_javaRuntimeObject, RELEASE_NATIVE_INSTANCE_METHOD_ID,
                         jsInstanceInfo->JavaObjectID());

    DetachGarbageCollectedWrapper(object);
}

void ObjectManager::MakeJavaInstanceWeak(jint javaObjectID) {
    JEnv env;
    env.CallVoidMethod(m_javaRuntimeObject, MAKE_INSTANCE_WEAK_METHOD_ID, javaObjectID);
}

bool ObjectManager::IsJavaInstanceAlive(jint javaObjectID) {
    JEnv env;
    jboolean isHeld = env.CallBooleanMethod(m_javaRuntimeObject, IS_INSTANCE_ALIVE_METHOD_ID, javaObjectID);
    return isHeld == JNI_TRUE;
}

static inline const char* GCTypeString(GCType type) {
    switch (type) {
        case kGCTypeScavenge: return "Nursery";
        case kGCTypeMinorMarkCompact: return "Minor";
        case kGCTypeMarkSweepCompact: return "Major";
        case kGCTypeIncrementalMarking: return "Incremental";
        case kGCTypeProcessWeakCallbacks: return "Process weak callbacks";
        case kGCTypeAll: assert(false && "kGCTypeAll should not be passed to GC callback");
    }
}

static inline size_t GCTypeIndex(GCType type) {
    switch (type) {
        case kGCTypeScavenge: return 0;
        case kGCTypeMinorMarkCompact: return 1;
        case kGCTypeMarkSweepCompact: return 2;
        case kGCTypeIncrementalMarking: return 3;
        case kGCTypeProcessWeakCallbacks: return 4;
        case kGCTypeAll: assert(false && "kGCTypeAll should not be passed to GC callback");
    }
}

void ObjectManager::OnGCStart(Isolate* isolate, GCType type, GCCallbackFlags, void *data) {
    auto* self = static_cast<ObjectManager*>(data);
    self->m_gcStartTime = std::chrono::steady_clock::now();

    if (type != kGCTypeMarkSweepCompact) return;
    for (const auto &[id, ref]: *self->m_idToObject) {
        auto* jsInfo = ExtractWrapper<JSInstanceInfo>(ref.Get(isolate));
        if (!jsInfo) continue;  // when can this happen?
        jsInfo->StartCountingTraces();
    }
}

void ObjectManager::OnGCFinish(Isolate* isolate, GCType type, GCCallbackFlags, void *data) {
    auto* self = static_cast<ObjectManager*>(data);

    if (type == kGCTypeMarkSweepCompact) {
        // Any Java object IDs in m_gcUnmarkedIDs with count < 2 are objects whose JS wrappers were
        // only traced once. This means that only m_idToObject was holding on to them, and they were not
        // reachable from JS.
        std::vector<jint> idsToClear;

        for (const auto &[id, jsObject]: *self->m_idToObject) {
            auto* jsInfo = ExtractWrapper<JSInstanceInfo>(jsObject.Get(isolate));
            if (!jsInfo) continue;  // when can this happen?
            if (!jsInfo->HasBeenTracedTwice()) {
                try {
                    self->MakeJavaInstanceWeak(id);
                    if (!self->IsJavaInstanceAlive(id)) {
                        // If the Java instance is dead, this JavaScript instance can be let die.
                        idsToClear.push_back(id);
                    }
                } catch (NativeScriptException& ex) {
                    // If the Java method throws an exception, there's not much we can do about it;
                    // there's nowhere else to throw it to. Log it and continue iterating.
                    ex.Log();
                }
            }
        }

        for (jint id : idsToClear) {
            self->m_idToObject->Drop(id);
        }
    }

    std::chrono::steady_clock::time_point gcEndTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::duration gcDuration = gcEndTime - self->m_gcStartTime;
    DEBUG_WRITE_FORCE("%s GC finished in %lld µs: %zu JS objects live", GCTypeString(type),
                      std::chrono::duration_cast<std::chrono::microseconds>(gcDuration).count(),
                      JSInstanceInfo::s_liveCount.load());
    size_t ix = GCTypeIndex(type);
    self->m_gcCounts[ix]++;
    self->m_gcDurations[ix] += gcDuration;
}

void ObjectManager::LogGCStats() {
    std::chrono::microseconds averages[5] = {0us, 0us, 0us, 0us, 0us};
    for (size_t ix = 0; ix < 5; ix++) {
        if (m_gcCounts[ix])
            averages[ix] = std::chrono::duration_cast<std::chrono::microseconds>(
                    m_gcDurations[ix] / m_gcCounts[ix]);
    }
    DEBUG_WRITE_FORCE("GC statistics:\n"
                      "- %zu nursery, average %lld µs\n"
                      "- %zu minor, average %lld µs\n"
                      "- %zu major, average %lld µs\n"
                      "- %zu incremental, average %lld µs\n"
                      "- %zu process weak callbacks, average %lld µs",
                      m_gcCounts[0], averages[0].count(),
                      m_gcCounts[1], averages[1].count(),
                      m_gcCounts[2], averages[2].count(),
                      m_gcCounts[3], averages[3].count(),
                      m_gcCounts[4], averages[4].count());
}
