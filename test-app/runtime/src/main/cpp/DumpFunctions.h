#pragma once

#include <string>

#include "v8-isolate.h"
#include "v8-local-handle.h"
#include "v8-object.h"
#include "v8-primitive.h"
#include "v8-value.h"

std::string DumpBigInt(v8::Local<v8::BigInt> b);
std::string DumpString(v8::Isolate *iso, v8::Local<v8::String> s);
std::string DumpSymbol(v8::Isolate *iso, v8::Local<v8::Symbol> s);
std::string DumpObject(v8::Local<v8::Object> o);
std::string DumpValue(v8::Isolate *iso, v8::Local<v8::Value> v);
