#include <vector>
#include <string>
// for uint32_t
#include <cstdint>
int add_pids_to_new_cgroup(std::vector<uint32_t> pids, std::string cgroup_name, double cpu_limit);
bool doesunitexist(std::string name);
std::string getunitpath(std::string name);
bool updatecgroup(std::vector<uint32_t> pids, std::string unitpath, double cpu_limit);