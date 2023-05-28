#include "plogformatter.hpp"

plog::util::nstring KWPlogFormatter::format(const plog::Record &record)
{
    plog::util::nostringstream ss;
    ss << std::setfill(PLOG_NSTR(' ')) << std::setw(5) << std::left << severityToString(record.getSeverity()) << PLOG_NSTR(" ");
    ss << PLOG_NSTR("[") << getFilename << PLOG_NSTR(":") << record.getLine() << PLOG_NSTR("] ");
    ss << record.getMessage() << PLOG_NSTR("\n");

    return ss.str();
}

plog::util::nstring KWPlogFormatter::header()
{
    return plog::util::nstring();
}