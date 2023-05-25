#pragma once

#include <jni.h>

#include <cppgc/garbage-collected.h>

namespace cppgc {
class Visitor;
}

namespace tns {
class ObjectManager;

class JSInstanceInfo final : public cppgc::GarbageCollected<JSInstanceInfo> {
public:
    static std::atomic<ssize_t> s_liveCount;

    JSInstanceInfo(jint javaObjectID, ObjectManager *om)
        : m_javaObjectID(javaObjectID), m_objManager(om) {
        ++s_liveCount;
    }
    ~JSInstanceInfo();

    jint JavaObjectID() const { return m_javaObjectID; }

    void Trace(cppgc::Visitor*) const {}

private:
    jint m_javaObjectID;
    ObjectManager *m_objManager;
};

} // namespace tns
