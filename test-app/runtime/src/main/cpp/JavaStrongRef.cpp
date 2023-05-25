#include <jni.h>

#include "JavaStrongRef.h"
#include "ObjectManager.h"

namespace tns {

JavaStrongRef::JavaStrongRef(jint javaObjectID, ObjectManager *om)
    : m_javaObjectID(javaObjectID), m_objectManager(om) {
    m_objectManager->HoldJavaInstance(javaObjectID);
}

JavaStrongRef::~JavaStrongRef() {
    m_objectManager->ReleaseJavaInstance(m_javaObjectID);
}

} // namespace tns
