#include <iostream>
#include <string>
#include <regex>
#include <filesystem>
#include <fstream>
#include <unordered_set>
#include <set>
#include <memory>

#include <qt6/QtCore/QCoreApplication>
#include <qt6/QtCore/QCommandLineParser>
#include "cgroups.hpp"

enum LPROCTYPE
{
    PROCESS,
    THREAD
};

class LProcess
{
public:
    uint32_t pid;
    LPROCTYPE type;
    std::string name;
    std::set<int> children;

    LProcess(int pid, LPROCTYPE type, std::string name)
    {
        this->pid = pid;
        this->type = type;
        this->name = name;
    }

    bool operator==(const LProcess &other) const
    {
        return this->pid == other.pid;
    }

    struct HashFunction
    {
        std::size_t operator()(const LProcess &proc) const
        {
            return std::hash<int>()(proc.pid);
        }
    };
};

enum class GETPID_METRICS
{
    TOP,
};
std::unique_ptr<LProcess> getpidfor(std::string name, GETPID_METRICS metric = GETPID_METRICS::TOP);
std::set<uint32_t> getchildrenpids(std::unique_ptr<LProcess> &proc);
std::set<uint32_t> getpidsforexec(std::string name);
std::set<uint32_t> getallchildren(std::set<uint32_t> pids);

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    // TODO: get these from the build system
    QCoreApplication::setApplicationName("battery-thing");
    QCoreApplication::setApplicationVersion("0.1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("A simple battery mode using cgroups to limit background processes.");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.process(app);

    // std::set<uint32_t> processpids = getpidsforexec("/usr/bin/stress");
    // std::string appname = "stress";
    std::set<uint32_t> processpids = getpidsforexec("/opt/discord/Discord");
    std::string appname = "discord";
    std::string unitname = "batterything-" + appname + ".scope";
    std::cout << "children: ";
    for (int procpid : processpids)
    {
        std::cout << procpid << " ";
    }

    std::cout << std::endl;

    std::set<uint32_t> childrenpids2 = getallchildren(processpids);
    std::cout << "children2: ";
    for (int procpid : childrenpids2)
    {
        std::cout << procpid << " ";
    }
    std::cout << std::endl;

    add_pids_to_new_cgroup(std::vector<uint32_t>(childrenpids2.begin(), childrenpids2.end()), unitname);
    std::cout << "done" << std::endl;
}

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
            continue;
        }
    }
    return pids;
}

std::unique_ptr<LProcess> getpidfor(std::string name, GETPID_METRICS metric)
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
            LProcess proc = LProcess(pid, LPROCTYPE::PROCESS, proc_name);
            return std::make_unique<LProcess>(proc);
        }
    }
    return nullptr;
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