#ifndef ZOOMRECAPP_H
#define ZOOMRECAPP_H

#include "AppBase.h"

class ZoomrecApp : public AppBase {
protected:
  void SetupApp(); // ovveride this if reqired
  void LoopApp();     // ovveride this if reqired
  void IntervalApp(); // ovveride this if reqired

private:
  const int inputPinResetSwitch = D1;
  const int outputPinPowerButton = D2;
  int lastStatus = LOW;

  time_t convertISO8601ToUnixTime(const char *isoTimestamp);
  const char *unixTimestampToISO8601(time_t unixTimestamp, char *iso8601Time, size_t bufferSize);
  bool isPCRunning();
  void togglePowerButton();
  void startPC();
  void shutDownPC();
  void checkUpdateStatus();
};

#endif // ZOOMRECAPP_H
