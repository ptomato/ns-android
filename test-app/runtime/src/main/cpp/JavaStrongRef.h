#pragma once

#include <jni.h>

#include <cppgc/garbage-collected.h>
#include <cppgc/visitor.h>

namespace tns {

class ObjectManager;

class JavaStrongRef final : public cppgc::GarbageCollected<JavaStrongRef> {
public:
    JavaStrongRef(jint javaObjectID, ObjectManager* om);
    ~JavaStrongRef();

    JavaStrongRef() = delete;
    JavaStrongRef(JavaStrongRef&) = delete;
    JavaStrongRef& operator=(JavaStrongRef&) = delete;
    JavaStrongRef(JavaStrongRef&&) = delete;
    JavaStrongRef& operator=(JavaStrongRef&&) = delete;

    void Trace(cppgc::Visitor*) const {};

private:
    ObjectManager *m_objectManager;
    jint m_javaObjectID;
};

} // namespace tns
