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
#include "processes.hpp"

enum class GETPID_METRICS
{
    TOP,
};

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

    parser.addPositionalArgument("executable", "The executable to set parameters for.");

    QCommandLineOption cpuLimit = QCommandLineOption(QStringList() << "c"
                                                                   << "cpu-limit",
                                                     "The maximum CPU usage in percent.", "percent", "50");
    parser.addOption(cpuLimit);

    parser.process(app);

    // is there an executable?
    if (parser.positionalArguments().size() < 1)
    {
        std::cerr << "No executable specified." << std::endl;
        return 1;
    }

    std::string apppath = parser.positionalArguments()[0].toStdString();
    std::string cpulimit = parser.value(cpuLimit).toStdString();

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

    std::string unitname = "batterything-" + appname + ".scope";

    std::cout << "apppath: " << apppathpathreal.string() << std::endl;
    std::cout << "appname: " << appname << std::endl;
    std::cout << "unitname: " << unitname << std::endl;

    double cpulimitdouble = std::stod(cpulimit) / 100.0;

    std::set<uint32_t> processpids = getpidsforexec(apppathpathreal.string());
    if (processpids.size() == 0)
    {
        std::cout << "no processes found" << std::endl;
        return 0;
    }
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

    if (doesunitexist(unitname))
    {
        std::cout << "updating cgroup..." << std::endl;
        updatecgroup(std::vector<uint32_t>(childrenpids2.begin(), childrenpids2.end()), unitname, cpulimitdouble);
        std::cout << "done" << std::endl;
    }
    else
    {
        std::cout << "creating cgroup..." << std::endl;
        add_pids_to_new_cgroup(std::vector<uint32_t>(childrenpids2.begin(), childrenpids2.end()), unitname, cpulimitdouble);
        std::cout << "done" << std::endl;
    }

    std::cout << "done" << std::endl;
}
