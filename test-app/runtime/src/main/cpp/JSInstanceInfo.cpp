#include <atomic>
#include <cassert>

#include "JSInstanceInfo.h"
#include "NativeScriptAssert.h"
#include "ObjectManager.h"

namespace cppgc {
class Visitor;
}

namespace tns {

JSInstanceInfo::~JSInstanceInfo() {
    m_objManager->MakeJavaInstanceWeak(m_javaObjectID);
    m_objManager->DropJSObjectByJavaObject(m_javaObjectID);

    ssize_t count = --s_liveCount;
    assert(count >= 0);
}

void JSInstanceInfo::Trace(cppgc::Visitor *visitor [[maybe_unused]]) const {
    if (m_traces > 0) m_traces--;
}

std::atomic<ssize_t> JSInstanceInfo::s_liveCount{0};

}  // namespace tns
