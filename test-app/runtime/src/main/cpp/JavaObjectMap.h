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
class ObjectManager;

/// JavaObjectMap is a CPPGC-traceable object which ensures that certain objects are kept alive
class JavaObjectMap final: public cppgc::GarbageCollected<JavaObjectMap> {
public:
    explicit JavaObjectMap(ObjectManager* om) : m_objManager(om), m_idToObject() {}
    JavaObjectMap() = delete;
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
    ObjectManager* m_objManager;
    std::unordered_map<jint, v8::TracedReference<v8::Object>> m_idToObject;
};

} // namespace tns
