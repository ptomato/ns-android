#pragma once

#include <string>

#include <v8-isolate.h>
#include <v8-local-handle.h>
#include <v8-value.h>

namespace tns {

namespace ConsoleMethod {
extern const std::string ASSERT;
extern const std::string DIR;
extern const std::string ERROR;
extern const std::string INFO;
extern const std::string LOG;
extern const std::string TIME_END;
extern const std::string TRACE;
extern const std::string WARNING;
}  // namespace ConsoleMethod

using ConsoleCallback = void (*)(v8::Isolate*, const std::string& method,
    const std::vector<v8::Local<v8::Value>>& args);

}  // namespace tns
