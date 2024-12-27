#pragma once

extern const int button_pin_a;
extern const int button_pin_b;
extern const int button_pin_c;
extern const int button_pin_d;
extern const int button_pin_e;

extern volatile bool state_a;
extern volatile bool state_b;
extern volatile bool state_c;
extern volatile bool state_d;
extern volatile bool state_e;

extern unsigned long lastPressTime_a;
extern unsigned long lastPressTime_b;
extern unsigned long lastPressTime_c;
extern unsigned long lastPressTime_d;
extern unsigned long lastPressTime_e;

extern const unsigned long debounceThreshold;

extern const unsigned long force_off_time_threshold;


inline void IRAM_ATTR handleButtonPress(int button_pin, volatile bool& state, unsigned long& lastPressTime) {
  unsigned long currentMillis = millis();
  if (currentMillis - lastPressTime >= debounceThreshold) {
    lastPressTime = currentMillis;
    state = digitalRead(button_pin);
  }
}

// Timer callback function to handle all buttons
inline void IRAM_ATTR handleAllButtons() {
  handleButtonPress(button_pin_a, state_a, lastPressTime_a);
  handleButtonPress(button_pin_b, state_b, lastPressTime_b);
  handleButtonPress(button_pin_c, state_c, lastPressTime_c);
  handleButtonPress(button_pin_d, state_d, lastPressTime_d);
  handleButtonPress(button_pin_e, state_e, lastPressTime_e);
}

inline void IRAM_ATTR checkForceOffTimeThreshold() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastPressTime_a >= force_off_time_threshold) {
    if (digitalRead(button_pin_a) == HIGH) {
      state_a = HIGH;
    }
  }

  if (currentMillis - lastPressTime_b >= force_off_time_threshold) {
    if (digitalRead(button_pin_b) == HIGH) {
      state_b = HIGH;
    }
  }

  if (currentMillis - lastPressTime_c >= force_off_time_threshold) {
    if (digitalRead(button_pin_c) == HIGH) {
      state_c = HIGH;
    }
  }

  if (currentMillis - lastPressTime_d >= force_off_time_threshold) {
    if (digitalRead(button_pin_d) == HIGH) {
      state_d = HIGH;
    }
  }

  if (currentMillis - lastPressTime_e >= force_off_time_threshold) {
    if (digitalRead(button_pin_e) == HIGH) {
      state_e = HIGH;
    }
  }
}
