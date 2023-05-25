#pragma once

#include <unordered_map>

#include <jni.h>

#include <cppgc/garbage-collected.h>
#include <v8-local-handle.h>
#include <v8-object.h>
#include <v8-traced-handle.h>

namespace cppgc {
class Visitor;
}

namespace v8 {
class Isolate;
}

namespace tns {

/// JavaObjectMap is a CPPGC-traceable object which ensures that certain objects are kept alive
class JavaObjectMap final: public cppgc::GarbageCollected<JavaObjectMap> {
public:
    JavaObjectMap() = default;
    ~JavaObjectMap() = default;

    JavaObjectMap(JavaObjectMap&) = delete;
    JavaObjectMap(JavaObjectMap&&) = delete;
    JavaObjectMap& operator=(JavaObjectMap&) = delete;
    JavaObjectMap& operator=(JavaObjectMap&&) = delete;

    void Add(v8::Isolate* isolate, jint javaObjectID, v8::Local<v8::Object> jsObject);
    v8::Local<v8::Object> Get(v8::Isolate* isolate, jint javaObjectID);
    void Drop(jint javaObjectID);
    void Trace(cppgc::Visitor* visitor) const;

private:
    std::unordered_map<jint, v8::TracedReference<v8::Object>> m_idToObject;
};

} // namespace tns
