#include <vector>
#include <string>
// for uint32_t
#include <cstdint>
#include <set>
int add_pids_to_new_cgroup(std::set<uint32_t> pids, std::string cgroup_name);
bool doesunitexist(std::string name);
std::string getunitpath(std::string name);
bool updatecgroup(std::string unitpath, double cpu_limit);
bool setgroupcpulimit(std::set<uint32_t> app_pids, std::string unitname, double cpulimit);