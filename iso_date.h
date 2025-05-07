#pragma once
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

time_t convertISO8601ToUnixTime(const char *isoTimestamp);
const char *convertUnixTimestampToISO8601(time_t unixTimestamp, char *iso8601Time, size_t bufferSize);

#ifdef __cplusplus
}
#endif
