#ifndef BATTERYTHING_PLOGFORMATTER_HPP
#define BATTERYTHING_PLOGFORMATTER_HPP

#include <plog/Util.h>
#include <plog/Record.h>
#include <iomanip>

class KWPlogFormatter
{
public:
    static plog::util::nstring header()
    {
        return plog::util::nstring();
    }

    static plog::util::nstring format(const plog::Record &record)
    {
        plog::util::nostringstream ss;
        ss << std::setfill(PLOG_NSTR(' ')) << std::setw(5) << std::left << severityToString(record.getSeverity()) << PLOG_NSTR(" ");
        ss << PLOG_NSTR("[") << record.getFilename() << PLOG_NSTR(":") << record.getLine() << PLOG_NSTR("] ");
        ss << record.getMessage() << PLOG_NSTR("\n");

        return ss.str();
    }
};

#endif // BATTERYTHING_PLOGFORMATTER_HPP