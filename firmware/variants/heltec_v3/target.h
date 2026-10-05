#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#include <HeltecV3Board.h>
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#ifndef UNIFI_MINIMAL
#include <helpers/AutoDiscoverRTCClock.h>
#endif
#include <helpers/SensorManager.h>
#ifndef UNIFI_MINIMAL
#include <helpers/sensors/EnvironmentSensorManager.h>
#endif
#ifdef DISPLAY_CLASS
  #include <helpers/ui/SSD1306Display.h>
  #include <helpers/ui/MomentaryButton.h>
#endif

extern HeltecV3Board board;
extern WRAPPER_CLASS radio_driver;
#ifdef UNIFI_MINIMAL
extern ESP32RTCClock rtc_clock;
extern SensorManager sensors;
#else
extern AutoDiscoverRTCClock rtc_clock;
extern EnvironmentSensorManager sensors;
#endif

#ifdef DISPLAY_CLASS
  extern DISPLAY_CLASS display;
  extern MomentaryButton user_btn;
#endif

bool radio_init();
mesh::LocalIdentity radio_new_identity();
