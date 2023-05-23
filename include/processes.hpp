#include <set>
#include <string>
#include <cstdint>
#include <filesystem>

uint32_t getpidfor(std::string name);
std::set<uint32_t> getchildrenpids(uint32_t pid);
std::set<uint32_t> getpidsforexec(std::string name);
std::set<uint32_t> getallchildren(std::set<uint32_t> pids);
std::pair<std::filesystem::path, std::string> getapppath(std::string apppath);