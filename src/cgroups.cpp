#include "cgroups.hpp"
#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>
#include <iostream>


int add_pids_to_new_cgroup(std::vector<uint32_t> pids, std::string cgroup_name)
{
    // https://www.freedesktop.org/wiki/Software/systemd/ControlGroupInterface/
    // https://unix.stackexchange.com/questions/525740/how-do-i-create-a-systemd-scope-for-an-already-existing-process-from-the-command
    // call freedesktop.systemd1.Manager StartTransientUnit with the PIDs
    // ssa(sv)a(sa(sv))

    const char* busname = "org.freedesktop.systemd1";
    const char* objectpath = "/org/freedesktop/systemd1";
    const char* interface = "org.freedesktop.systemd1.Manager";
    const char* method = "StartTransientUnit";

    // create a proxy object using a system bus connection
    std::cout<<"Creating proxy..."<<std::endl;
    auto connection = sdbus::createSystemBusConnection();
    auto proxy = sdbus::createProxy(*connection, busname, objectpath);
    auto method_call = proxy->createMethodCall(interface, method);

    std::cout<<"created method call"<<std::endl;


    const char* failmode = "fail";
    int numprops = 1;
    const char* propname = "PIDs";

    method_call << cgroup_name << failmode;

    // a(sv) is an array of structs, each struct has a string and a variant
    // here, the string is "PIDs" and the variant is an array of ints

    auto pid_props = sdbus::Struct<std::string, std::vector<unsigned int>>(propname, pids);
    // method_call << std::vector<sdbus::Struct<std::string, sdbus::Variant>>({pid_props});
    auto props_array = std::vector<sdbus::Struct<std::string, sdbus::Variant>>({pid_props});
    // limit CPU
    // 200 ms, or 20% of one cpu
    double percent = 1.0;
    unsigned long quota = percent * 1000000; // if percent=0.2, then quota=200000
    props_array.push_back(sdbus::Struct<std::string, sdbus::Variant>("CPUQuotaPerSecUSec", quota));
    method_call << props_array;

    // rest is unused
    std::vector<sdbus::Struct<std::string, std::vector<sdbus::Struct<std::string, sdbus::Variant>>>> props2;
    method_call << props2;

    std::cout<<"Calling method..."<<std::endl;
    auto result = proxy->callMethod(method_call);
    std::cout<<"Method called."<<std::endl;

    return 0;
}