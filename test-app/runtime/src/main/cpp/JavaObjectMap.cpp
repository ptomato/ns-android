#include <unordered_map>
#include <utility>

#include <jni.h>

#include <v8-cppgc.h>
#include <v8-local-handle.h>
#include <v8-object.h>
#include <v8-traced-handle.h>

#include "JavaObjectMap.h"

namespace cppgc {
class Visitor;
}

namespace tns {
using namespace v8;

void JavaObjectMap::Add(Isolate* isolate, jint javaObjectID, Local<Object> jsObject) {
    m_idToObject.emplace(javaObjectID, TracedReference<Object>{isolate, jsObject});
}

Local<Object> JavaObjectMap::Get(Isolate* isolate, jint javaObjectID) {
    auto it = m_idToObject.find(javaObjectID);
    if (it == m_idToObject.end()) {
        return {};
    }

    return it->second.Get(isolate);
}

void JavaObjectMap::Drop(jint javaObjectID) {
    m_idToObject.erase(javaObjectID);
}

void JavaObjectMap::Trace(cppgc::Visitor* visitor) const {
    auto* jsVisitor = reinterpret_cast<JSVisitor*>(visitor);
    for (auto& entry : m_idToObject) {
        jsVisitor->Trace(entry.second);
    }
}

} // namespace tns
