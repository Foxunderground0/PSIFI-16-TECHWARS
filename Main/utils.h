#pragma once
#if SERIAL_ENABLE
#define SerialPrintLn(x) Serial.println(x)
#define SerialPrintLn2(x, y) Serial.println(x, y)
#define SerialPrint(x) Serial.print(x)
#else
#define SerialPrintLn(x)
#define SerialPrintLn2(x, y)
#define SerialPrint(x)
#endif
