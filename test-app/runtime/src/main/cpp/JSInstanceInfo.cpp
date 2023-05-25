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
    DEBUG_WRITE_FORCE(",,, Collecting JSInstanceInfo %p for %d", this, m_javaObjectID);
    ssize_t count = --s_liveCount;
    assert(count >= 0);
}

std::atomic<ssize_t> JSInstanceInfo::s_liveCount{0};

}  // namespace tns
