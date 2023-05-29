#ifndef BATTERYTHING_PLOGFORMATTER_HPP
#define BATTERYTHING_PLOGFORMATTER_HPP

#include <plog/Util.h>
#include <plog/Record.h>
#include <iomanip>
#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>

// check if record has a getFilename() method
#ifdef PLOG_HAVE_FILENAME
#define getFilename record.getFilename()
#else
#define getFilename record.getFile()
#endif

class KWPlogFormatter
{
public:
    static plog::util::nstring header();

    static plog::util::nstring format(const plog::Record &record);
};

static plog::ColorConsoleAppender<KWPlogFormatter> consoleAppender;

#endif // BATTERYTHING_PLOGFORMATTER_HPP