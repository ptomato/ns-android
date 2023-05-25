#pragma once

#include <cassert>
#include <cstdint>
#include <type_traits>

#include <cppgc/allocation.h>
#include <cppgc/garbage-collected.h>
#include <v8-cppgc.h>
#include <v8-isolate.h>
#include <v8-local-handle.h>
#include <v8-object.h>
#include <v8-value.h>

namespace tns {

static inline cppgc::AllocationHandle& allocationHandleForIsolate(v8::Isolate* i) {
    return i->GetCppHeap()->GetAllocationHandle();
}

template <typename T, typename... Args>
T* MakeGarbageCollected(v8::Isolate* isolate, Args&&... args) {
    if constexpr (std::is_constructible_v<T, v8::Isolate*, Args&&...>) {
        // Supply the isolate* to the target constructor if it's the first parameter.
        return cppgc::MakeGarbageCollected<T>(allocationHandleForIsolate(isolate), isolate, std::forward<Args>(args)...);
    }
    return cppgc::MakeGarbageCollected<T>(allocationHandleForIsolate(isolate), std::forward<Args>(args)...);
}

static constexpr uint16_t kGarbageCollectedEmbedderId = 5440;
static constexpr v8::WrapperDescriptor kGarbageCollectedWrapperDescriptor {
        0, /* wrappable_type_index, field which holds Smi-encoded kGarbageCollectedEmbedderId */
        1, /* wrappable_instance_index, field which holds a BaseDataWrapper* or other GC type */
        kGarbageCollectedEmbedderId,
};
static constexpr int kMinGarbageCollectedEmbedderFields = 2;
static_assert(kMinGarbageCollectedEmbedderFields >=
              std::max(
                      kGarbageCollectedWrapperDescriptor.wrappable_type_index,
                      kGarbageCollectedWrapperDescriptor.wrappable_instance_index));

template <typename T, typename = std::enable_if_t<cppgc::IsGarbageCollectedOrMixinTypeV<T>>>
static inline bool AttachGarbageCollectedWrapper(v8::Local<v8::Object> object, T* p) {
    alignas(2) static const uint16_t embedderMagic{kGarbageCollectedEmbedderId};
    auto* wrappableType = const_cast<uint16_t*>(&embedderMagic);

    assert(object->InternalFieldCount() >= kMinGarbageCollectedEmbedderFields);

    const v8::WrapperDescriptor& desc = kGarbageCollectedWrapperDescriptor;
    object->SetAlignedPointerInInternalField(desc.wrappable_type_index, wrappableType);
    object->SetAlignedPointerInInternalField(desc.wrappable_instance_index, p);

    return true;
}

static inline void DetachGarbageCollectedWrapper(v8::Local<v8::Object> object) {
    assert(object->InternalFieldCount() >= kMinGarbageCollectedEmbedderFields);

    const v8::WrapperDescriptor& desc = kGarbageCollectedWrapperDescriptor;
    object->SetAlignedPointerInInternalField(desc.wrappable_type_index, nullptr);
    object->SetAlignedPointerInInternalField(desc.wrappable_instance_index, nullptr);
}

static inline bool IsGarbageCollectedWrapper(v8::Local<v8::Object> object) {
    if (object.IsEmpty() || object->InternalFieldCount() < kMinGarbageCollectedEmbedderFields) {
        return false;
    }
    const v8::WrapperDescriptor& desc = kGarbageCollectedWrapperDescriptor;
    auto* p = static_cast<uint16_t*>(object->GetAlignedPointerFromInternalField(desc.wrappable_type_index));
    return p && *p == kGarbageCollectedEmbedderId;
}

template <typename T, typename = std::enable_if_t<cppgc::IsGarbageCollectedOrMixinTypeV<T>>>
T* ExtractWrapper(v8::Local<v8::Object> object)
{
    if (!IsGarbageCollectedWrapper(object))
        return nullptr;

    const v8::WrapperDescriptor& desc = kGarbageCollectedWrapperDescriptor;
    return static_cast<T*>(object->GetAlignedPointerFromInternalField(desc.wrappable_instance_index));
}

}  // namespace tns
