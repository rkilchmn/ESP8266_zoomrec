#ifndef CONSOLE_H
#define CONSOLE_H

#include <Arduino.h>

class Console : public Stream
{
public:
    enum LogLevel
    {
        DEBUG = 10,
        INFO = 20,
        WARNING = 30,
        ERROR = 40,
        CRITICAL = 50
    };

    // Constructor
    Console();

    // Required Stream functions to implement
    virtual size_t write(uint8_t data) override;
    virtual void flush() override;
    virtual int available() override;
    virtual int read() override;
    virtual int peek() override;

    // Additional functions from Stream that can be implemented if needed
    // ...

    // Function to begin with Serial
    void begin(unsigned long baudRate);

    // Function to begin with a stream e.g. TelnetStream
    void begin(Stream &primary, Stream &secondOutput);

    // Function to begin with a stream e.g. TelnetStream
    void begin(Stream &primary);

    void setLogLevel(LogLevel level);

    static const char *getLogLevelString(LogLevel level);

    LogLevel intToLogLevel(int intValue);

    void log(LogLevel level, const __FlashStringHelper *format, ...);

protected:
    Stream *primaryStream;
    Stream *SecondaryOutputStream;     // backup for output e.g. keep sending to Serial
    LogLevel logLevelThreshold = INFO; // Default log level is DEBUG
    
};

#endif // CONSOLE_H
