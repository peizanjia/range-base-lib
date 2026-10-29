#include <cstdlib>
#include <cmath>

#include "rbl_progress.h"
#include "rbl_logger.h"


RProgress::RProgress()
    : pulseType(false)
    , lastFraction(0.0)
    , printToLog(false)
    , progressHandler(nullptr)
    , progressInitializeHandler(nullptr)
    , progressFinalizeHandler(nullptr)
{
} /* RProgress:: */


RProgress::RProgress(const RProgress &progress)
{
    this->_init(&progress);
} /* RProgress::RProgress(copy) */


RProgress::~RProgress()
{
} /* RProgress::~RProgress */


RProgress & RProgress::operator =(const RProgress &progress)
{
    this->_init(&progress);
    return(*this);
} /* RProgress::operator = */


void RProgress::_init(const RProgress *pProgress)
{
    if (pProgress)
    {
        this->pulseType    = pProgress->pulseType;
        this->lastFraction = pProgress->lastFraction;
        this->setPrintToLog(pProgress->getPrintToLog());
        this->setProgressPrintHandler(pProgress->getProgressPrintHandler());
        this->setProgressInitializeHandler(pProgress->getProgressInitializeHandler());
        this->setProgressFinalizeHandler(pProgress->getProgressFinalizeHandler());
    }
} /* RProgress::_init(const RProgress *pProgress) */


RProgress & RProgress::getInstance()
{
    static RProgress progress;
    return progress;
} /* RProgress::getInstance */


bool RProgress::getPrintToLog() const
{
    return this->printToLog;
} /* RProgress::getPrintToLog */


void RProgress::setPrintToLog(bool printToLog)
{
    this->printToLog = printToLog;
} /* RProgress::setPrintToLog */


RProgressPrintHandler RProgress::getProgressPrintHandler() const
{
    return this->progressHandler;
} /* RProgress::getProgressHandler */


void RProgress::setProgressPrintHandler(RProgressPrintHandler progressHandler)
{
    this->progressHandler = progressHandler;
} /* RProgress::setProgressHandler */


RProgressInitializeHandler RProgress::getProgressInitializeHandler() const
{
    return this->progressInitializeHandler;
} /* RProgress::getProgressInitializeHandler */


void RProgress::setProgressInitializeHandler(RProgressInitializeHandler progressInitializeHandler)
{
    this->progressInitializeHandler = progressInitializeHandler;
} /* RProgress::setProgressInitializeHandler */


RProgressFinalizeHandler RProgress::getProgressFinalizeHandler() const
{
    return this->progressFinalizeHandler;
} /* RProgress::getProgressFinalizeHandler */


void RProgress::setProgressFinalizeHandler(RProgressFinalizeHandler progressFinalizeHandler)
{
    this->progressFinalizeHandler = progressFinalizeHandler;
} /* RProgress::setProgressFinalizeHandler */


void RProgress::initialize(const QString &message, bool pulseType)
{
    this->pulseType = pulseType;
    this->lastFraction = 0.0;
    this->print(0.0);
    if (this->printToLog)
    {
        bool addNewLine = RLogger::getInstance().getAddNewLine();
        RLogger::getInstance().setAddNewLine(false);
        RLogger::getInstance().print(message + " ");
        RLogger::getInstance().setAddNewLine(addNewLine);
    }
    if (this->progressInitializeHandler)
    {
        this->progressInitializeHandler(message,this->pulseType);
    }
} /* RProgress::initialize */


void RProgress::finalize(const QString &message)
{
    this->print(1.0);
    if (this->printToLog)
    {
        bool addNewLine = RLogger::getInstance().getAddNewLine();
        RLogger::getInstance().setAddNewLine(false);
        RLogger::getInstance().print(" " + message + "\n");
        RLogger::getInstance().setAddNewLine(addNewLine);
    }
    if (this->progressFinalizeHandler)
    {
        this->progressFinalizeHandler(message);
    }
} /* RProgress::finalize */


void RProgress::print(double fraction)
{
    if (this->printToLog)
    {
        bool addNewLine = RLogger::getInstance().getAddNewLine();
        RLogger::getInstance().setAddNewLine(false);
        if (this->pulseType)
        {
            uint indentLevel = RLogger::getInstance().getIndentLevel();
            RLogger::getInstance().setIndentLevel(0);
            RLogger::getInstance().print('.');
            RLogger::getInstance().setIndentLevel(indentLevel);
        }
        else
        {
            uint nMarks =(uint)floor(fraction * 10);
            uint nLastMarks =(uint)floor(this->lastFraction * 10);
            for (uint i=nLastMarks;i<nMarks;i++)
            {
                uint indentLevel = RLogger::getInstance().getIndentLevel();
                RLogger::getInstance().setIndentLevel(0);
                RLogger::getInstance().print("#");
                RLogger::getInstance().setIndentLevel(indentLevel);
            }
            if (nMarks-nLastMarks >= 1)
            {
                this->lastFraction = fraction;
            }
        }
        RLogger::getInstance().setAddNewLine(addNewLine);
    }
    if (this->progressHandler)
    {
        this->progressHandler(fraction);
    }
} /* RProgress::print */


void RProgressInitialize(const QString &message, bool pulseType)
{
    RProgress::getInstance().initialize(message,pulseType);
} /* RProgressInitialize */


void RProgressFinalize(const QString &message)
{
    RProgress::getInstance().finalize(message);
} /* RProgressFinalize */


void RProgressPrint(double fraction)
{
    RProgress::getInstance().print(fraction);
} /* RProgressPrint */


void RProgressPrint(uint64_t step, uint64_t nSteps)
{
    // An empty work list is already complete. Avoid sending NaN to the
    // logger's progress-mark count when there are no element pairs.
    RProgress::getInstance().print(nSteps == 0 ? 1.0 : double(step)/double(nSteps));
} /* RProgressPrint */


void RProgressPrintToLog(bool printToLog)
{
    RProgress::getInstance().setPrintToLog(printToLog);
} /* RProgressPrintToLog */
