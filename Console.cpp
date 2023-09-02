#include "Console.h"
#include <Arduino.h>

// Constructor
Console::Console()
{
    primaryStream = &Serial;
    SecondaryOutputStream = nullptr;
}

// Required Stream functions to implement
size_t Console::write(uint8_t data)
{
    // write to secondary output
    if (SecondaryOutputStream != nullptr)
        SecondaryOutputStream->write(data);

    // Print the data to the current stream
    return primaryStream->write(data);
}

int Console::available()
{
    // Check the availability of input data on the current stream
    return primaryStream->available();
}

int Console::read()
{
    // Read a byte of data from the current stream
    return primaryStream->read();
}

int Console::peek()
{
    // Peek at the next byte of input data from the current stream
    return primaryStream->peek();
}

// Function to begin with Serial
void Console::begin(unsigned long baudRate)
{
    Serial.begin(baudRate);
    primaryStream = &Serial;
}

// Function to begin with a stream e.g. TelnetStream
void Console::begin(Stream &primary, Stream &secondOutput)
{
    primaryStream = &primary;
    SecondaryOutputStream = &secondOutput;
}

// Function to begin with a stream e.g. TelnetStream
void Console::begin(Stream &primary)
{
    primaryStream = &primary;
}

void Console::setLogLevel(LogLevel level)
{
    logLevelThreshold = level;
}

const char *Console::getLogLevelString(LogLevel level)
{
    switch (level)
    {
    case DEBUG:
        return "DEBUG";
    case INFO:
        return "INFO";
    case WARNING:
        return "WARNING";
    case ERROR:
        return "ERROR";
    case CRITICAL:
        return "CRITICAL";
    default:
        return "UNKNOWN";
    }
}

Console::LogLevel Console::intToLogLevel(int intValue)
{
    switch (intValue)
    {
    case 10:
        return DEBUG;
    case 20:
        return INFO;
    case 30:
        return WARNING;
    case 40:
        return ERROR;
    case 50:
        return CRITICAL;
    default:
        return DEBUG; // Default to DEBUG if the value is not recognized
    }
}

void Console::log(LogLevel level, const __FlashStringHelper *format, ...)
{
    if (level >= logLevelThreshold)
    {
        const char *fmt = reinterpret_cast<const char *>(format);

        time_t localTime = time(nullptr);
        struct tm *tm = localtime(&localTime);

        // Create a temp string using strftime() function
        char temp[100];
        strftime(temp, sizeof(temp), "%Y-%m-%d %H:%M:%S ", tm);
        print(temp);

        // log level
        print(getLogLevelString(level));
        print(" ");

        va_list arg;
        va_start(arg, format);
        vsnprintf(temp, sizeof(temp), fmt, arg);
        va_end(arg);
        print(temp);
        print("\n");
    }
}