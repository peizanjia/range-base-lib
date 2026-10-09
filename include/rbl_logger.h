#ifndef RBL_LOGGER_H
#define RBL_LOGGER_H

#include <QFile>
#include <QStack>
#include <QString>
#include <QList>
#include <QElapsedTimer>
#include <QTextStream>
#include <QThread>

#include <vector>

#include "rbl_message.h"

#ifdef _MSC_VER
#define R_FUNCTION __FUNCSIG__
#else
#define R_FUNCTION __PRETTY_FUNCTION__
#endif

#define R_LOG_LEVEL_NORMAL static_cast<RLogLevelMask>(RMessage::Type::Info | RMessage::Type::Notice | RMessage::Type::Error)
#define R_LOG_LEVEL_QUIET static_cast<RLogLevelMask>(RMessage::Type::Error | RMessage::Type::Warning)
#define R_LOG_LEVEL_DETAIL static_cast<RLogLevelMask>(RMessage::Type::Info | RMessage::Type::Notice | RMessage::Type::Error | RMessage::Type::Warning)
#define R_LOG_LEVEL_DEBUG  static_cast<RLogLevelMask>(RMessage::Type::Info | RMessage::Type::Notice | RMessage::Type::Error | RMessage::Type::Warning | RMessage::Type::Debug)
#define R_LOG_LEVEL_TRACE  static_cast<RLogLevelMask>(RMessage::Type::Info | RMessage::Type::Notice | RMessage::Type::Error | RMessage::Type::Warning | RMessage::Type::Debug | RMessage::Type::Trace)

#define R_LOG_TRACE RLogger::trace("(Thread: %p) %s @ %d\n",QThread::currentThread(),__FILE__,__LINE__)
#define R_LOG_TRACE_MESSAGE(_message) RLogger::trace("(Thread: %p) %s @ %d: %s\n",QThread::currentThread(),__FILE__,__LINE__,_message)
#define R_LOG_TRACE_IN \
{ \
    if (RLogger::getInstance().getLevel() == R_LOG_LEVEL_TRACE) \
    { \
        RLogger::indent(); \
        RLogger::trace("(Thread: %p) %s @ %d (%s) +\n",QThread::currentThread(),__FILE__,__LINE__,R_FUNCTION); \
    } \
}
#define R_LOG_TRACE_OUT \
{ \
    if (RLogger::getInstance().getLevel() == R_LOG_LEVEL_TRACE) \
    { \
        RLogger::trace("(Thread: %p) %s @ %d -\n",QThread::currentThread(),__FILE__,__LINE__); \
        RLogger::unindent(false); \
    } \
}

#define R_LOG_TRACE_RETURN(_retVal) { R_LOG_TRACE_OUT; return _retVal; }

#define R_LOG_LEVEL_IS_VALID(_level) \
(                                    \
  (_level == R_LOG_LEVEL_QUIET)  ||  \
  (_level == R_LOG_LEVEL_NORMAL) ||  \
  (_level == R_LOG_LEVEL_DETAIL) ||  \
  (_level == R_LOG_LEVEL_DEBUG)  ||  \
  (_level == R_LOG_LEVEL_TRACE)      \
)

typedef int RLogLevelMask;
typedef RMessage::Type RLogLevel;

typedef void (*RLogHandler)(const RMessage &message);

//! Logger class.
class RLogger
{

    private:

        //! Internal initialization function.
        void _init(const RLogger *pLogger = nullptr);

        //! Open log file for appending. Closes any previously open file first.
        void openLogFile() const;

        //! Close log file if open.
        void closeLogFile() const;

        //! Print to log file.
        void printToFile(qint64 pTime, const QString &cppString) const;

        //! Insert prefix string to message.
        static void insertPrefix(const QString &prefix, QString &message);

    protected:

        //! List of unprocessed messages.
        std::vector<RMessage> messages;
        //! Log file name.
        QString logFileName;
        //! Persistent log file handle.
        mutable QFile *logFile = nullptr;
        //! Text stream over logFile.
        mutable QTextStream *logStream = nullptr;
        //! Log level.
        RLogLevelMask logLevel;
        //! Logger halted state.
        //! If set to halt then all messages will be stored in history and
        //! printed after the logger will be unhalted.
        bool halted;
        //! If set to true a message assign time will be printed into
        //! a log file.
        bool printTime;
        //! If set to true a message assign thread ID will be printed into
        //! a log file.
        bool printThreadId;
        //! Append new line character to ech string.
        bool addNewLine;
        //! Custom log handler.
        RLogHandler logHandler;
        //! Current indent level.
        uint indentLevel;
        //! Time measurement stack.
        QStack<QElapsedTimer> timerStack;
        //! Stack of logger prefixes.
        QStack<QString> prefixStack;

    public:

        //! Constructor
        RLogger(RLogLevelMask logLevel = R_LOG_LEVEL_DETAIL);

        //! Copy constructor.
        RLogger(const RLogger &logger);

        //! Destructor.
        ~RLogger();

        //! Assignment operator.
        RLogger & operator =(const RLogger &logger);

        //! Return static instance.
        static RLogger & getInstance();

        //! Return log level.
        RLogLevelMask getLevel() const;

        //! Set log level.
        void setLevel(RLogLevelMask level);

        //! Return halt state of the logger.
        bool getHalted() const;

        //! Halt/Unhalt the logger.
        //! If halted messages will not be printed into a file.
        void setHalted(bool halt);

        //! Return true/false whether the time will be printed into
        //! a log file.
        bool getPrintTimeEnabled() const;

        //! Enable/disable print time into a log file
        void setPrintTimeEnabled(bool printTime);

        //! Return true/false whether the thread ID will be printed into
        //! a log file.
        bool getPrintThreadIdEnabled() const;

        //! Enable/disable print thread ID into a log file
        void setPrintThreadIdEnabled(bool printThreadId);

        //! Return whether a new line char should be added.
        bool getAddNewLine() const;

        //! Set whether a new line char should be added.
        void setAddNewLine(bool addNewLine);

        //! Return log file name.
        QString getFile() const;

        //! Set log file name.
        void setFile(const QString & logFileName);

        //! Return custom log handler.
        RLogHandler getLogHandler() const;

        //! Set custom log handler.
        void setLogHandler(RLogHandler logHandler);

        //! Return current indent level.
        uint getIndentLevel() const;

        //! Set current indent level.
        void setIndentLevel(uint indentLevel);

        //! Increase current indent level.
        void increaseIndent();

        //! Decrease current indent level.
        void decreaseIndent();

        //! Print message.
        void print(const RMessage &message);

        //! Print c++ string
        void print(const QString &cppString, RMessage::Type messageType = RMessage::Type::Info);

        //! Print c string
        void print(const char *cString, RMessage::Type messageType = RMessage::Type::Info);

        //! Print c char
        void print(const char cChar, RMessage::Type messageType = RMessage::Type::Info);

        //! Flush all unprocessed messages to file.
        void flush();

        //! Purge unprocessed messages.
        //! If nMessages is set to 0 all messages will be purged.
        void purge(uint nMessages = 0);

        //! Convenience function to log trace message.
        static int trace(const char *format, ...);

        //! Convenience function to log debug message.
        static int debug(const char *format, ...);

        //! Convenience function to log info message.
        static int info(const char *format, ...);

        //! Convenience function to log notice message.
        static int notice(const char *format, ...);

        //! Convenience function to log warning message.
        static int warning(const char *format, ...);

        //! Convenience function to log error message.
        static int error(const char *format, ...);

        //! Convenience function to print timestamp.
        static void timestamp(const QString &prefix = QString());

        //! Convenience function to increase indent.
        static void indent();

        //! Convenience function to decrease indent.
        static void unindent(bool printTime = true);

        //! Convenience function to push prefix.
        static void pushPrefix(const QString &prefix);

        //! Convenience function to pop current prefix.
        static void popPrefix();

        //! Install Qt message handler to route Qt logs through RLogger.
        static void installQtMessageHandler();

};

#endif /* RBL_LOGGER_H */
