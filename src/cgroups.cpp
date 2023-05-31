#include "cgroups.hpp"
#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>
#include <iostream>
#include <set>
#include "plogformatter.hpp"
#include <plog/Log.h>

#define busname "org.freedesktop.systemd1"
#define objectpath "/org/freedesktop/systemd1"
#define interface "org.freedesktop.systemd1.Manager"

int add_pids_to_new_cgroup(std::set<uint32_t> pids, std::string cgroup_name)
{
    // https://www.freedesktop.org/wiki/Software/systemd/ControlGroupInterface/
    // https://unix.stackexchange.com/questions/525740/how-do-i-create-a-systemd-scope-for-an-already-existing-process-from-the-command
    // call freedesktop.systemd1.Manager StartTransientUnit with the PIDs
    // ssa(sv)a(sa(sv))

    const char* method = "StartTransientUnit";

    // create a proxy object using a system bus connection
    auto connection = sdbus::createSystemBusConnection();
    auto proxy = sdbus::createProxy(*connection, busname, objectpath);
    auto method_call = proxy->createMethodCall(interface, method);

    const char *failmode = "fail";
    const char* propname = "PIDs";

    method_call << cgroup_name << failmode;

    // a(sv) is an array of structs, each struct has a string and a variant
    // here, the string is "PIDs" and the variant is an array of ints

    auto pid_props = sdbus::Struct<std::string, std::vector<unsigned int>>(propname, std::vector<unsigned int>(pids.begin(), pids.end()));
    auto props_array = std::vector<sdbus::Struct<std::string, sdbus::Variant>>({pid_props});
    method_call << props_array;

    // rest is unused
    std::vector<sdbus::Struct<std::string, std::vector<sdbus::Struct<std::string, sdbus::Variant>>>> props2;
    method_call << props2;

    LOG_VERBOSE << "creating unit " << cgroup_name << " with PIDs " << pids;
    auto result = proxy->callMethod(method_call);

    return 0;
}

bool updatecgroup(std::string unitpath, double cpu_limit)
{
    const char *method = "SetUnitProperties";
    // sba(sv)
    // s: name, b: runtime, a(sv): properties
    // runtime controls whether the changes are saved to disk
    // if true, don't save to disk

    auto connection = sdbus::createSystemBusConnection();
    auto proxy = sdbus::createProxy(*connection, busname, objectpath);
    auto method_call = proxy->createMethodCall(interface, method);

    method_call << unitpath << true;

    // as an aside: setting PIDs is an append operation
    // to clear the PIDs, set it to an empty array first, then append

    // auto pid_props = sdbus::Struct<std::string, std::vector<unsigned int>>("PIDs", pids);
    // auto props_array = std::vector<sdbus::Struct<std::string, sdbus::Variant>>({pid_props});
    auto props_array = std::vector<sdbus::Struct<std::string, sdbus::Variant>>();
    // set to 100% of one cpu
    unsigned long quota = cpu_limit * 1000000; // if percent=0.2, then quota=200000
    // props_array.push_back(sdbus::Struct<std::string, sdbus::Variant>("CPUQuotaPerSecUSec", quota));
    // to disable CPU limits, set quota to max 64 bit unsigned long
    unsigned long max_quota = UINT64_MAX;
    if (cpu_limit == 0)
        quota = max_quota;
    props_array.push_back(sdbus::Struct<std::string, sdbus::Variant>("CPUQuotaPerSecUSec", quota));
    method_call << props_array;

    LOG_VERBOSE << "updating unit " << unitpath << " with CPU limit " << cpu_limit;
    auto result = proxy->callMethod(method_call);

    return true;
}

std::string getunitpath(std::string name)
{
    const char *method = "GetUnit";
    auto connection = sdbus::createSystemBusConnection();
    auto proxy = sdbus::createProxy(*connection, busname, objectpath);
    auto method_call = proxy->createMethodCall(interface, method);

    // arguments: s

    method_call << name;
    sdbus::ObjectPath path;

    try
    {

        auto result = proxy->callMethod(method_call);
        // output: o
        // o is the object path of the unit
        result >> path;
        return std::string(path);
    }
    catch (sdbus::Error &e)
    {
        // if the unit doesn't exist, the call will fail
        // make sure the error is "org.freedesktop.systemd1.NoSuchUnit"
        if (e.getName() == "org.freedesktop.systemd1.NoSuchUnit")
        {
            return "";
        }
        else
        {
            LOG_WARNING << "Error: " << e.getName() << std::endl;
            LOG_WARNING << e.getMessage() << std::endl;
            return "";
        }
    }
}

bool doesunitexist(std::string name)
{
    return getunitpath(name) != "";
}

bool setgroupcpulimit(std::set<uint32_t> app_pids, std::string unitname, double cpulimit)
{
    if (!doesunitexist(unitname))
    {
        add_pids_to_new_cgroup(app_pids, unitname);
    }
    updatecgroup(unitname, cpulimit);
    // TODO: make sure the above worked
    return true;
}