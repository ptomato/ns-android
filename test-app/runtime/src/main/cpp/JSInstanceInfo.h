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

    void Trace(cppgc::Visitor* visitor) const;

    void StartCountingTraces() { m_traces = 2; }
    bool HasBeenTracedTwice() { return m_traces == 0; }

private:
    jint m_javaObjectID;
    ObjectManager *m_objManager;
    mutable unsigned m_traces;
};

} // namespace tns
