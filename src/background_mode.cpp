#include "windowfocus.hpp"
#include "cgroups.hpp"
#include "processes.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <thread>
#include <chrono>

std::vector<std::string> apps = {};
std::vector<std::string> appnames = {};
std::map<std::string, std::set<uint32_t>> app_pids;

void update_pids()
{
    // update PIDs
    for (auto app : apps)
    {
        std::set<uint32_t> pids = getpidsforexec(app);
        std::set<uint32_t> all_pids = getallchildren(pids);
        all_pids.insert(pids.begin(), pids.end());
        app_pids[app] = all_pids;
    }
}

int main(int argc, char **argv)
{
    std::cout << "starting background mode" << std::endl;
    WindowFocusDetector wfd;

    std::vector<std::string> apps_in = {"/usr/bin/discord", "/usr/bin/slack"};
    for (auto app : apps_in)
    {
        std::pair<std::filesystem::path, std::string> apppaths = getapppath(app);
        std::filesystem::path apppathpathreal = apppaths.first;
        apps.push_back(apppathpathreal.string());
        appnames.push_back(apppaths.second);
    }
    std::cout << "resolved apps: " << std::endl;
    for (auto app : apps)
    {
        std::cout << app << std::endl;
    }
    update_pids();
    std::chrono::time_point<std::chrono::system_clock> last_pid_update = std::chrono::system_clock::now();

    // set up initial cgroups
    for (size_t i = 0; i < apps.size(); i++)
    {
        std::string unitname = "batterything-" + appnames[i] + ".scope";
        if (doesunitexist(unitname))
        {
            std::cout << "updating cgroup for " << apps[i] << std::endl;
            updatecgroup(std::vector<uint32_t>(app_pids[apps[i]].begin(), app_pids[apps[i]].end()), unitname, 0.5);
        }
        else
        {
            std::cout << "creating cgroup for " << apps[i] << std::endl;
            add_pids_to_new_cgroup(std::vector<uint32_t>(app_pids[apps[i]].begin(), app_pids[apps[i]].end()), unitname, 0.5);
        }
    }
    std::string last_active_app = "";
    int last_active_app_index = -1;
    while (true)
    {
        std::string active_app = "";
        int active_app_index = -1;
        // update PIDs every 10 seconds
        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - last_pid_update).count() > 10)
        {
            std::cout << "updating pids" << std::endl;
            update_pids();
            last_pid_update = std::chrono::system_clock::now();
        }
        uint32_t activepid = wfd.getActiveWindowPID();
        std::cout << "active pid: " << activepid << std::endl;
        // check if active pid is in any of the app_pids
        for (size_t i = 0; i < apps.size(); i++)
        {
            if (app_pids[apps[i]].find(activepid) != app_pids[apps[i]].end())
            {
                active_app = apps[i];
                active_app_index = i;
            }
        }
        // did we switch out of a managed app?
        if (active_app == "")
        {
            std::cout << "active pid is not in any managed apps" << std::endl;
            if (last_active_app != "")
            {
                std::cout << "switched out of " << last_active_app << std::endl;
                std::string unitname = "batterything-" + appnames[last_active_app_index] + ".scope";
                updatecgroup(std::vector<uint32_t>(app_pids[last_active_app].begin(), app_pids[last_active_app].end()), unitname, 0.5);
            }
        }
        else
        {
            if (last_active_app != active_app)
            {
                std::cout << "switched to " << active_app << std::endl;
                std::string unitname = "batterything-" + appnames[active_app_index] + ".scope";
                updatecgroup(std::vector<uint32_t>(app_pids[active_app].begin(), app_pids[active_app].end()), unitname, 2.0);
            }
        }
        last_active_app = active_app;
        last_active_app_index = active_app_index;
        // if switched to a new app, update cgroups
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}