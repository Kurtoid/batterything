#include "processes.hpp"
#include <string>
#include <fstream>
#include <regex>
#include <filesystem>
#include <set>
#include "plogformatter.hpp"
#include <plog/Log.h>

std::string getstatline(std::filesystem::path stat_path)
{
    std::ifstream stat_file(stat_path);
    std::string stat_line;
    std::getline(stat_file, stat_line);
    stat_file.close();
    return stat_line;
}
std::string getprocname(std::string stat_line)
{
    std::regex stat_regex("\\(([^\\)]+)\\)");
    std::smatch stat_match;
    std::regex_search(stat_line, stat_match, stat_regex);
    if (stat_match.size() < 2)
    {
        return "";
    }
    std::string proc_name = stat_match[1];
    return proc_name;
}

std::set<uint32_t> getpidsforexec(std::string name)
{
    // walk through /proc/*/exe and check if it's symlinked to the name
    std::set<uint32_t> pids;
    std::filesystem::path path("/proc");
    for (const auto &entry : std::filesystem::directory_iterator(path))
    {
        try
        {
            // is this a /proc/pid, or something else?
            std::string exe_path = entry.path() / "exe";
            if (!std::filesystem::exists(exe_path))
            {
                continue;
            }
            std::filesystem::path exe_symlink = std::filesystem::read_symlink(exe_path);
            if (exe_symlink == name)
            {
                // this is the process we're looking for
                pids.insert(atoi(entry.path().filename().c_str()));
            }
        }
        catch (std::filesystem::filesystem_error &e)
        {
            // this is not a /proc/pid, or permission denied
            PLOG_DEBUG << "error reading " << entry.path() << ": " << e.what();
            continue;
        }
    }
    return pids;
}

uint32_t getpidfor(std::string name)
{
    // search for the process name in /proc
    // multiple processes may have the same name name, so we use the metric to
    // determine which one to return

    // keep a hashset of pids we've already checked
    // std::unordered_set<int> checked_pids;

    // list folders in /proc
    std::filesystem::path path("/proc");
    std::regex proc_regex("^[0-9]+$");
    std::smatch proc_match;
    for (const auto &entry : std::filesystem::directory_iterator(path))
    {
        // std::cout << entry.path() << std::endl;
        // check stat
        std::string stat_line = getstatline(entry.path() / "stat");
        std::string proc_name = getprocname(stat_line);
        // get the pid
        int pid = atoi(entry.path().filename().c_str());
        if (proc_name != name)
        {
            continue;
        }

        // get the ppid from stat (field 4)
        int ppid;
        // TODO: optimize this regex
        std::regex stat_regex("^[^ ]+ [^ ]+ [^ ]+ ([^ ]+) ");
        std::smatch stat_match;
        std::regex_search(stat_line, stat_match, stat_regex);
        if (stat_match.size() < 2)
        {
            continue;
        }
        ppid = atoi(stat_match[1].str().c_str());
        // get the name of the parent process
        std::string pstat_line = getstatline(std::filesystem::path("/proc") / std::to_string(ppid) / "stat");
        std::string pproc_name = getprocname(pstat_line);

        if (pproc_name != name)
        {
            // this is the process we're looking for
            return pid;
        }
    }
    return 0;
}

std::set<uint32_t> getchildrenpids(uint32_t pid)
{
    // /proc/pid/task/pid/children recursively
    std::set<uint32_t> childrenpids;
    std::deque<uint32_t> pids_to_check;
    pids_to_check.push_back(pid);
    while (!pids_to_check.empty())
    {
        uint32_t pid = pids_to_check.front();
        // std::cout << "checking " << pid << std::endl;
        pids_to_check.pop_front();
        std::filesystem::path path("/proc");
        path /= std::to_string(pid);
        path /= "task";
        try
        {
            // find subfolders
            for (const auto &entry : std::filesystem::directory_iterator(path))
            {
                // std::cout << entry.path() << std::endl;
                if (entry.is_directory())
                {
                    uint32_t childpid = atoi(entry.path().filename().c_str());
                    if (childpid == 0)
                    {
                        continue;
                    }
                    if (childrenpids.find(childpid) != childrenpids.end())
                    {
                        continue;
                    }
                    childrenpids.insert(childpid);
                    pids_to_check.push_back(childpid);
                }
            }
        }
        catch (std::filesystem::filesystem_error &e)
        {
            // tasks can be short-lived, so it's possible that the task folder
            // doesn't exist anymore
            PLOG_DEBUG << "couldn't list tasks for " << pid << ": " << e.what();
            continue;
        }
    }
    return childrenpids;
}

std::set<uint32_t> getallchildren(std::set<uint32_t> pids)
{
    std::set<uint32_t> childrenpids;
    for (uint32_t pid : pids)
    {
        std::set<uint32_t> childpids = getchildrenpids(pid);
        childrenpids.insert(childpids.begin(), childpids.end());
    }
    return childrenpids;
}

std::pair<std::filesystem::path, std::string> getapppath(std::string apppath)
{
    std::filesystem::path apppathpath(apppath);
    // follow symlinks

    std::filesystem::path apppathpathreal = apppathpath;
    while (std::filesystem::is_symlink(apppathpathreal))
    {
        apppathpathreal = std::filesystem::read_symlink(apppathpathreal);
    }
    // symlink might be relative, so make it absolute. if it is relative, it's relative to the symlink's directory
    if (!apppathpathreal.is_absolute())
    {
        apppathpathreal = apppathpath.parent_path() / apppathpathreal;
        apppathpathreal = std::filesystem::canonical(apppathpathreal);
    }
    // get application name
    std::string appname = apppathpathreal.filename().string();
    // make appname lowercase
    std::transform(appname.begin(), appname.end(), appname.begin(), ::tolower);

    // remove dashes and underscores
    appname.erase(std::remove(appname.begin(), appname.end(), '-'), appname.end());
    appname.erase(std::remove(appname.begin(), appname.end(), '_'), appname.end());

    return std::make_pair(apppathpathreal, appname);
}
