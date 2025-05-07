#include "iso_date.h"
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include "date.h" // Howard Hinnant's date library

extern "C" {
#include <time.h>
}

time_t convertISO8601ToUnixTime(const char *isoTimestamp) {
    using namespace date;
    using namespace std;
    using namespace std::chrono;

    istringstream in{isoTimestamp};
    sys_time<microseconds> tp;

    // Try with fractional seconds first
    in >> parse("%Y-%m-%dT%H:%M:%S.%f%Ez", tp);
    if (in.fail()) {
        // Clear fail state and try without fractional seconds
        in.clear();
        in.str(isoTimestamp); // reset input stream
        in >> parse("%Y-%m-%dT%H:%M:%S%Ez", tp);
        if (in.fail()) {
            return (time_t)-1; // still failed
        }
    }

    return system_clock::to_time_t(time_point_cast<seconds>(tp));
}

const char *convertUnixTimestampToISO8601(time_t unixTimestamp, char *iso8601Time, size_t bufferSize)
{
  struct tm *tmStruct = localtime(&unixTimestamp);
  snprintf(iso8601Time, bufferSize, "%04d-%02d-%02dT%02d:%02d:%02d",
           tmStruct->tm_year + 1900, tmStruct->tm_mon + 1, tmStruct->tm_mday,
           tmStruct->tm_hour, tmStruct->tm_min, tmStruct->tm_sec);
  return iso8601Time;
}
