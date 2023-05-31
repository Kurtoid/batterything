#include "windowfocus.hpp"
#include "cgroups.hpp"
#include "processes.hpp"
#include "plogformatter.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <thread>
#include <chrono>
// signal handling
#include <csignal>
#include <plog/Init.h>
#include <plog/Log.h>

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
        PLOG_INFO << "Found " << all_pids.size() << " PIDs for " << app;
        std::string pids_str = "";
        for (auto pid : all_pids)
        {
            pids_str += std::to_string(pid) + ", ";
        }
        PLOG_VERBOSE << "PIDs for " << app << ": " << pids_str;
    }
}

int main(int argc, char **argv)
{
    // set up logging
    plog::init(plog::verbose, &consoleAppender);

    PLOG_INFO << "starting batterything";
    PLOG_INFO << "PLOG_HAVE_FILENAME: " << PLOG_HAVE_FILENAME;

    // handle control-c
    signal(SIGINT, [](int signum)
           {
            PLOG_INFO << "caught SIGINT, exiting";
            // we can't easily remove the cgroups, so just set the limit to 0
            for (size_t i = 0; i < apps.size(); i++)
            {
                try{
                std::string unitname = "batterything-" + appnames[i] + ".scope";
                // TODO: this function can create the cgroup if it doesn't exist, which isn't ideal since we're exiting
                setgroupcpulimit(app_pids[apps[i]], unitname, 0);
                }
                catch(std::exception &e)
                {
                    PLOG_ERROR << "error while removing cgroup for " << apps[i] << ": " << e.what();
                }
            }
            PLOG_INFO << "exiting";
        exit(0); });

    WindowFocusDetector wfd;

    std::vector<std::string> apps_in = {"/usr/bin/slack", "/usr/lib/firefox-developer-edition/firefox", "/usr/bin/discord"};
    for (auto app : apps_in)
    {
        std::pair<std::filesystem::path, std::string> apppaths = getapppath(app);
        std::filesystem::path apppathpathreal = apppaths.first;
        apps.push_back(apppathpathreal.string());
        appnames.push_back(apppaths.second);
        PLOG_VERBOSE << "added " << app << " as " << apppaths.second;
    }

    update_pids();
    std::chrono::time_point<std::chrono::system_clock> last_pid_update = std::chrono::system_clock::now();

    // set up initial cgroups
    for (size_t i = 0; i < apps.size(); i++)
    {
        if (app_pids[apps[i]].size() == 0)
        {
            continue;
        }
        std::string unitname = "batterything-" + appnames[i] + ".scope";
        setgroupcpulimit(app_pids[apps[i]], unitname, 0.5);
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
            PLOG_VERBOSE << "updating PIDs";
            update_pids();
            last_pid_update = std::chrono::system_clock::now();
        }
        uint32_t activepid = wfd.getActiveWindowPID();
        PLOG_VERBOSE << "active pid: " << activepid;
        // check if active pid is in any of the app_pids
        for (size_t i = 0; i < apps.size(); i++)
        {
            if (app_pids[apps[i]].find(activepid) != app_pids[apps[i]].end())
            {
                active_app = apps[i];
                active_app_index = i;
                break;
            }
        }
        // did we switch out of a managed app?
        if (active_app == "")
        {
            PLOG_VERBOSE << "active pid is not in any managed apps";
            if (last_active_app != "")
            {
                // last active app may have been closed, so this could fail
                PLOG_INFO << "switched out of " << last_active_app;
                std::string unitname = "batterything-" + appnames[last_active_app_index] + ".scope";
                try
                {
                    setgroupcpulimit(app_pids[last_active_app], unitname, 0.5);
                }
                catch (std::exception &e)
                {
                    PLOG_WARNING << "failed to update cgroup for " << last_active_app << ": " << e.what();
                }
            }
        }
        else
        {
            if (last_active_app != active_app)
            {
                PLOG_INFO << "switched to " << active_app;
                std::string unitname = "batterything-" + appnames[active_app_index] + ".scope";
                setgroupcpulimit(app_pids[active_app], unitname, 0);
            }
        }
        last_active_app = active_app;
        last_active_app_index = active_app_index;
        // if switched to a new app, update cgroups
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}
