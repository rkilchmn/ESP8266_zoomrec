struct lockerStatus_t
{
  unsigned int byteLength; // if 0 not valid
  unsigned int checkSum;
} lockerStatus;//     void
//     debugLockerStatus(lockerStatus_t lockerStatus)
// {
// #ifdef DEBUG
//   Serial.println("lockerStatus.byteLength=" + String(lockerStatus.byteLength));
//   Serial.println("lockerStatus.wifi.ssid=" + String(lockerStatus.wifi.ssid));
//   Serial.println("lockerStatus.wifi.key=" + String(lockerStatus.wifi.key));
// #endif
// }

// void setlockerStatus(lockerStatus_t lockerStatus)
// {
//   mem.write(MEMORY_OFFSET_LOCKERSTATUS, (byte *)&lockerStatus, sizeof(lockerStatus));
// }

// lockerStatus_t getlockerStatus()
// {
//   lockerStatus_t lockerStatus;
//   // read from eeprom
//   mem.read(MEMORY_OFFSET_LOCKERSTATUS, (byte *)&lockerStatus, sizeof(lockerStatus));

//   unsigned int checkSum = calclockerStatusCheckSum(lockerStatus);
//   if (checkSum != lockerStatus.checkSum)
//   {
//     // null structure
//     lockerStatus = (struct lockerStatus_t){0};
//   }

//   return lockerStatus;
// }

// unsigned int calclockerStatusCheckSum(lockerStatus_t lockerStatus)
// {
//   unsigned int sum = 0;
//   unsigned char *p = (unsigned char *)&lockerStatus;
//   for (int i = 0; i < sizeof(lockerStatus); i++)
//   {
//     sum += p[i];
//   }
//   return sum;
// }
