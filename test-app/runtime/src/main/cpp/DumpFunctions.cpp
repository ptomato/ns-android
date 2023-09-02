#include <cstdint>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>

#include "v8-local-handle.h"
#include "v8-isolate.h"
#include "v8-object.h"
#include "v8-primitive.h"
#include "v8-value.h"

#include "DumpFunctions.h"

using v8::BigInt;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::String;
using v8::Symbol;
using v8::Value;

std::string DumpBigInt(Local<BigInt> b) {
    bool fits;
    int64_t iVal = b->Int64Value(&fits);
    if (fits) {
        return std::to_string(iVal) + "n";
    }
    uint64_t uVal = b->Uint64Value(&fits);
    if (fits) {
        return std::to_string(uVal) + "n";
    }

    int size = b->WordCount();
    int sign;
    std::unique_ptr<uint64_t[]> buf(new uint64_t[size]); // NOLINT(modernize-avoid-c-arrays)
    b->ToWordsArray(&sign, &size, buf.get());

    std::stringstream str{sign == 1 ? "-0x" : "0x"};
    str << std::hex << std::setfill('0') << std::setw(sizeof(uint64_t) * 2);
    for (int ix = 0; ix < size; ix++) {
        str << buf[ix];
    }
    str << "n";
    return str.str();
}

std::string DumpString(Isolate* iso, Local<String> s) {
    int encodedSize = s->Utf8Length(iso);
    std::unique_ptr<char[]> buf(new char[encodedSize]); // NOLINT(modernize-avoid-c-arrays)
    s->WriteUtf8(iso, buf.get(), encodedSize, nullptr, String::WriteOptions::NO_NULL_TERMINATION);
    return {buf.get(), size_t(encodedSize)};
}

std::string DumpSymbol(Isolate* iso, Local<Symbol> s) {
    Local<Value> desc = s->Description(iso);
    if (desc->IsUndefined()) {
        return "Symbol()";
    }
    assert(desc->IsString() && "Symbol description should be undefined or a string?");
    std::string descString = DumpString(iso, desc.As<String>());
    return "Symbol(" + descString + ")";
}

std::string DumpObject(Local<Object> o) {
    return "Object " + std::to_string(o->GetIdentityHash());
}

std::string DumpValue(Isolate* iso, Local<Value> v) {
    if (v->IsUndefined()) {
        return "undefined";
    }
    if (v->IsNull()) {
        return "null";
    }
    if (v->IsTrue()) {
        return "true";
    }
    if (v->IsFalse()) {
        return "false";
    }
    if (v->IsNumber()) {
        Local<Number> n = v.As<Number>();
        return std::to_string(n->Value());
    }
    if (v->IsBigInt()) {
        Local<BigInt> b = v.As<BigInt>();
        return DumpBigInt(b);
    }
    if (v->IsString()) {
        Local<String> s = v.As<String>();
        return DumpString(iso, s);
    }
    if (v->IsSymbol()) {
        Local<Symbol> s = v.As<Symbol>();
        return DumpSymbol(iso, s);
    }
    if (v->IsObject()) {
        Local<Object> o = v.As<Object>();
        return DumpObject(o);
    }
    return "unhandled, typeof is " + DumpString(iso, v->TypeOf(iso));
}
