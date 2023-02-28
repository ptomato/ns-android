#include <string>

#include <src/inspector/protocol/Runtime.h>

namespace tns {

namespace protocol = v8_inspector::protocol;

namespace ConsoleMethod {
const std::string ASSERT{protocol::Runtime::ConsoleAPICalled::TypeEnum::Assert};
const std::string DIR{protocol::Runtime::ConsoleAPICalled::TypeEnum::Dir};
const std::string ERROR{protocol::Runtime::ConsoleAPICalled::TypeEnum::Error};
const std::string INFO{protocol::Runtime::ConsoleAPICalled::TypeEnum::Info};
const std::string LOG{protocol::Runtime::ConsoleAPICalled::TypeEnum::Log};
const std::string TIME_END{protocol::Runtime::ConsoleAPICalled::TypeEnum::TimeEnd};
const std::string TRACE{protocol::Runtime::ConsoleAPICalled::TypeEnum::Trace};
const std::string WARNING{protocol::Runtime::ConsoleAPICalled::TypeEnum::Warning};
}  // namespace ConsoleMethod

}  // namespace tns
