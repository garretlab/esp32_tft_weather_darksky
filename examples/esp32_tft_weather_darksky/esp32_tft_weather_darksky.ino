#include <WiFi.h>
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "JsonListener.h"
#include "DarkskyParser.h"
#include "Fonts/FreeSans9pt7b.h"

DarkskyParser dsParser;

const char *ssid = "SSID";
const char *password = "PASSWORD";

const char *apiKey = "APIKEY";
const char *latitude = "LATITUDE";
const char *longitude = "LONGITUDE";

const char *tz = "JST-9";
const char *ntpServer1 = "ntp.nict.jp";
const char *ntpServer2 = "time.google.com";
const char *ntpServer3 = "ntp.jst.mfeed.ad.jp";

/* weather definition */
enum {
  CLEAR_DAY = 0, CLEAR_NIGHT = 1, CLOUDY = 2,
  RAIN = 3, HEAVY_RAIN = 4,
  SNOW = 5, SLEET = 6, WIND = 7, FOG = 8,
  PARTLY_CLOUDY_DAY = 9, PARTLY_CLOUDY_NIGHT = 10,
  UNAVAILABLE = 11, OTHER = 12, INITIAL = 13,
};

/* icon difinition for each weather */
uint8_t weatherIcon[][32] = {
  /* 0: clear-day */
  {
    0x00, 0x00, 0x01, 0x80, 0x09, 0x90, 0x07, 0xe0,
    0x2f, 0xf4, 0x1f, 0xf8, 0x1f, 0xf8, 0x7f, 0xfe,
    0x7f, 0xfe, 0x1f, 0xf8, 0x1f, 0xf8, 0x2f, 0xf4,
    0x07, 0xe0, 0x09, 0x90, 0x01, 0x80, 0x00, 0x00,
  },
  /* 1: clear-night */
  {
    0x00, 0x00, 0x0f, 0xc0, 0x1f, 0xf0, 0x20, 0xf8,
    0x40, 0x7c, 0x00, 0x3c, 0x00, 0x1e, 0x00, 0x1e,
    0x00, 0x1e, 0x00, 0x1e, 0x00, 0x3c, 0x40, 0x7c,
    0x20, 0xf8, 0x1f, 0xf0, 0x0f, 0xc0, 0x00, 0x00,
  },
  /* 2: cloudy */
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x0e, 0x70, 0x1f, 0xf8, 0x3f, 0xfc,
    0x7f, 0xfe, 0x7f, 0xfe, 0x3f, 0xfc, 0x1f, 0xf8,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  },
  /* 3: rain */
  {
    0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x0f, 0xf0,
    0x1f, 0xf8, 0x3f, 0xfc, 0x7f, 0xfe, 0x41, 0x82,
    0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
    0x09, 0x80, 0x09, 0x80, 0x07, 0x00, 0x00, 0x00,
  },
  /* 4: heavy rain */
  {
    0x00, 0x00, 0x03, 0xf0, 0x27, 0xc0, 0x1f, 0x90,
    0x1f, 0x08, 0x3e, 0x44, 0x7f, 0x20, 0x7b, 0x90,
    0x71, 0xc0, 0x64, 0xe0, 0x42, 0x70, 0x51, 0x30,
    0x08, 0x18, 0x04, 0x90, 0x00, 0x60, 0x00, 0x00,
  },
  /* 5: snow */
  {
    0x00, 0x00, 0x07, 0xe0, 0x0f, 0xf0, 0x1d, 0xb8,
    0x1d, 0xb8, 0x1f, 0xf8, 0x0c, 0x30, 0x07, 0xe0,
    0x1f, 0xf8, 0x3f, 0xfc, 0x7f, 0xfe, 0x7f, 0xfe,
    0x7f, 0xfe, 0x3f, 0xfc, 0x1f, 0xf8, 0x00, 0x00,
  },
  /* 6: sleet/fog(small cloud) */
  {
    0x00, 0x00, 0x0e, 0x70, 0x1f, 0xf8, 0x3f, 0xfc,
    0x7f, 0xfe, 0x7f, 0xfe, 0x3f, 0xfc, 0x1f, 0xf8,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  },
  /* 7: wind */
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0,
    0x01, 0x20, 0x01, 0xa0, 0x18, 0x20, 0x25, 0xcc,
    0x34, 0x12, 0x04, 0x1a, 0x78, 0x02, 0x03, 0xfc,
    0x00, 0x00, 0x7f, 0xfc, 0x00, 0x00, 0x00, 0x00,
  },
  /* 8: partly cloudy(cloud part) */
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x1f, 0x80,
    0x3f, 0xc0, 0x7f, 0xe0, 0x7f, 0xe0, 0x3f, 0xc0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  },
  /* 9: sleet(snow part) */
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x08, 0x00, 0x6b, 0x00, 0x1c, 0x00,
    0x1c, 0x00, 0x6b, 0x00, 0x08, 0x00, 0x00, 0x00,
  },
  /* 10: sleet(rain part) */
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x48, 0x00, 0x48, 0x00, 0x24,
    0x00, 0x24, 0x00, 0x12, 0x00, 0x12, 0x00, 0x00,
  },
  /* 11: fog(fog part) */
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00,
    0x1f, 0xf8, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0x00,
  },
  /* 12: unavailable */
  {
    0x00, 0x00, 0x07, 0xe0, 0x0f, 0xf0, 0x18, 0x18,
    0x31, 0x8c, 0x61, 0x86, 0x61, 0x86, 0x61, 0x86,
    0x61, 0x86, 0x60, 0x06, 0x61, 0x86, 0x31, 0x8c,
    0x18, 0x18, 0x0f, 0xf0, 0x07, 0xe0, 0x00, 0x00,
  },
  /* 13: other */
  {
    0x00, 0x00, 0x03, 0xc0, 0x07, 0xe0, 0x0c, 0x30,
    0x18, 0x18, 0x18, 0x18, 0x18, 0x30, 0x00, 0x60,
    0x00, 0xc0, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
    0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00,
  },
};

uint8_t kanji[][32] = {
  /* 気 */
  {
    0x10, 0x00, 0x10, 0x00, 0x3f, 0xfe, 0x20, 0x00,
    0x6f, 0xf8, 0xc0, 0x00, 0x3f, 0xf8, 0x00, 0x08,
    0x00, 0x48, 0x18, 0xC8, 0x0D, 0x88, 0x07, 0x08,
    0x0d, 0x8d, 0x18, 0xc5, 0x30, 0x47, 0xe0, 0x02,
  },
  /* 温 */
  {
    0x00, 0x00, 0x63, 0xfc, 0x32, 0x04, 0x12, 0x04,
    0x03, 0xfc, 0xC2, 0x04, 0x62, 0x04, 0x23, 0xfc,
    0x00, 0x00, 0x17, 0xfe, 0x14, 0x92, 0x34, 0x92,
    0x24, 0x92, 0x64, 0x92, 0x44, 0x92, 0xcf, 0xff,
  },
  /* 降 */
  {
    0x00, 0x40, 0x7c, 0x40, 0x44, 0xfc, 0x4d, 0x84,
    0x4B, 0x48, 0x58, 0x30, 0x50, 0xdc, 0x53, 0x87,
    0x48, 0x10, 0x49, 0xfe, 0x48, 0x90, 0x48, 0x90,
    0x5B, 0xff, 0x40, 0x10, 0x40, 0x10, 0x40, 0x10,
  },
  /* 水 */
  {
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x04,
    0xfd, 0x0c, 0x05, 0x98, 0x05, 0xb0, 0x0D, 0xc0,
    0x09, 0x40, 0x19, 0x60, 0x11, 0x30, 0x31, 0x18,
    0x61, 0x0C, 0xc1, 0x07, 0x01, 0x00, 0x07, 0x00,
  },
  /* 確 */
  {
    0x00, 0x20, 0x00, 0x20, 0xfc, 0xff, 0x11, 0x21,
    0x11, 0x65, 0x30, 0x48, 0x20, 0xfe, 0x3d, 0x90,
    0x66, 0x90, 0x64, 0xfe, 0xa4, 0x90, 0x24, 0x90,
    0x24, 0xfe, 0x24, 0x90, 0x3c, 0x90, 0x00, 0xff,
  },
  /* 率 */
  {
    0x01, 0x00, 0x01, 0x00, 0x7f, 0xfe, 0x02, 0x00,
    0x64, 0x4C, 0x32, 0x98, 0x01, 0x00, 0x12, 0x48,
    0x37, 0xac, 0x60, 0x06, 0x01, 0x00, 0xff, 0xff,
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
  },
  /* 風 */
  {
    0x00, 0x00, 0x3f, 0xf8, 0x20, 0x08, 0x20, 0xe8,
    0x2f, 0x88, 0x21, 0x08, 0x2f, 0xe8, 0x29, 0x28,
    0x29, 0x28, 0x2f, 0xe8, 0x21, 0x08, 0x21, 0x08,
    0x21, 0x2C, 0x63, 0xf5, 0x5e, 0x17, 0xc0, 0x02,
  },
  /* 速 */
  {
    0x00, 0x20, 0x60, 0x20, 0x37, 0xff, 0x10, 0x20,
    0x03, 0xfe, 0x02, 0x22, 0x02, 0x22, 0xf2, 0x22,
    0x13, 0xfe, 0x10, 0xa8, 0x11, 0xac, 0x13, 0x26,
    0x16, 0x23, 0x30, 0x20, 0x68, 0x00, 0xc7, 0xff,
  },
};

/* weather and icon mapping. displayed by the order and should be end with -1 */
struct weatherIconMapping_t {
  int icon[4];
  int iconColor[4];
} weatherIconMapping[] = {
  {{0, -1}, {0xfc44, -1}},                        /* 0: clear-day */
  {{1, -1}, {0xf7a6, -1}},                        /* 1: clear-night */
  {{2, -1}, {0xce79, -1}},                        /* 2: cloudy */
  {{3, -1}, {0x2377, -1}},                        /* 3: rain */
  {{4, -1}, {0x19ad, -1}},                        /* 4: heavy rain */
  {{5, -1}, {0xffff, -1}},                        /* 5: snow */
  {{6, 9, 10, -1}, {0xce79, 0xffff, 0x2377, -1}}, /* 6: sleet */
  {{7, -1}, {0xce79, -1}},                        /* 7: wind */
  {{6, 11, -1}, {0xce79, 0xce79, -1}},            /* 8: fog */
  {{0, 8, -1}, {0xfc44, 0xf7de, -1}},             /* 9: partly-cloudy-day */
  {{1, 8, -1}, {0xf7a6, 0xf7de, -1}},             /* 10: partly-cloudy-night */
  {{12, -1}, {0xf800, -1}},                       /* 11: unavailable */
  {{13, -1}, {0xffff, -1}},                       /* 12: other */
};

/* Pin assignment */
const int stmpeCs = 25;
const int tftCs = 32;
const int tftDc = 33;

Adafruit_ILI9341 tft = Adafruit_ILI9341(tftCs, tftDc);

const char degreeSymbol = 247;

float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void getMinMax(float x[], int num, float *minX, float *maxX) {
  *minX = *maxX = x[0];

  for (int i = 0; i < num; i++) {
    if (*minX > x[i]) {
      *minX = x[i];
    }
    if (*maxX < x[i]) {
      *maxX = x[i];
    }
  }
}

void getMinMax(int x[], int num, int *minX, int *maxX) {
  *minX = *maxX = x[0];

  for (int i = 0; i < num; i++) {
    if (*minX > x[i]) {
      *minX = x[i];
    }
    if (*maxX < x[i]) {
      *maxX = x[i];
    }
  }
}

/* mapping from darksky weather to this clock weather */
/* rain is split into usual rain and heavy rain */
int darkskyWeatherToIcon(int weather, int precipIntensity) {
  switch (weather) {
    case 0: /* clear-day */
      return CLEAR_DAY;
      break;
    case 1: /* clear-night */
      return CLEAR_NIGHT;
      break;
    case 2: /* rain */
      if (precipIntensity < 5) {
        return RAIN;
      } else {
        return HEAVY_RAIN;
      }
      break;
    case 3: /* snow */
      return SNOW;
      break;
    case 4: /* sleet */
      return SLEET;
      break;
    case 5:
      return WIND;
      break;
    case 6:
      return FOG;
      break;
    case 7: /* cloudy */
      return CLOUDY;
      break;
    case 8: /* partly-cloudy-day */
      return PARTLY_CLOUDY_DAY;
      break;
    case 9: /* partly-cloudy-night */
      return PARTLY_CLOUDY_NIGHT;
      break;
    default:
      return OTHER;
      break;
  }
}

/* draw weather icon at specified hour position */
void drawWeatherIcon(int weather[]) {
  /* delete old icon */
  for (int i = 0; i < 12; i++) {
    int n = 0;
    while (weatherIconMapping[weather[i]].icon[n] != -1) {
      tft.drawBitmap(23 * i + 26, 50,
                     weatherIcon[weatherIconMapping[weather[i]].icon[n]],
                     16, 16, weatherIconMapping[weather[i]].iconColor[n], ILI9341_BLACK);
      n++;
    }
  }
}

/* draw graph lines and percent numbers */
void drawGraph() {
  /* 0 - 100 % */
  tft.setTextColor(ILI9341_BLUE, ILI9341_BLACK);
  for (int i = 0; i < 6; i++) {
    tft.setCursor(300, 176 - 20 * i);
    tft.printf("%-3d", i * 20);
  }
  tft.setCursor(300, 66);
  tft.printf("  %%");

  /* horizontal line */
  for (int i = 0; i < 6; i++) {
    tft.drawLine(22, 80 + 20 * i, 298, 80 + 20 * i, 0x2986);
  }
  /* vertical line */
  for (int i = 0; i < 13; i++) {
    tft.drawLine(22 + 23 * i, 80, 22 + 23 * i, 180, 0x2986);
  }
}

/* draw precipitation probability */
void drawPrecipProbabilityGraph(int precipProbability[]) {
  for (int i = 0; i < 12; i ++) {
    tft.fillRect(23 * i + 24, map(precipProbability[i], 0, 100, 180, 80), 20, precipProbability[i], 0x961d);
  }
}

void drawPrecipProbabilityText(int precipProbability[]) {
  int minProb, maxProb;

  getMinMax(precipProbability, 12, &minProb, &maxProb);

  tft.fillRect(20, 202, 280, 16, ILI9341_BLACK);
  tft.drawBitmap(10, 202, kanji[2], 16, 16, ILI9341_WHITE, ILI9341_BLACK);
  tft.drawBitmap(26, 202, kanji[3], 16, 16, ILI9341_WHITE, ILI9341_BLACK);
  tft.drawBitmap(42, 202, kanji[4], 16, 16, ILI9341_WHITE, ILI9341_BLACK);
  tft.drawBitmap(58, 202, kanji[5], 16, 16, ILI9341_WHITE, ILI9341_BLACK);
  tft.setFont(&FreeSans9pt7b);
  tft.setCursor(80, 216);
  tft.printf(": %3d %% / %3d %% / %3d%%", precipProbability[0], minProb, maxProb);
  tft.setFont();
}

/* draw temperature */
void drawTemperatureGraph(float temperature[]) {
  float minTemp, maxTemp;
  int baseMinTemp, baseMaxTemp, tempStep;

  getMinMax(temperature, 12, &minTemp, &maxTemp);

  /* set temperature range to draw */
  for (int i = 0; i < 4; i++) {
    int step[] = {1, 2, 5, 10};
    tempStep = step[i];
    baseMinTemp = floor(minTemp / tempStep) * tempStep;
    baseMaxTemp = baseMinTemp + tempStep * 5;
    if ((baseMaxTemp >= maxTemp) && (baseMinTemp <= minTemp)) {
      break;
    }
  }

  for (int i = 0; i < 6; i++) {
    tft.setCursor(0, 176 - 20 * i);
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.print("   ");
    tft.setCursor(0, 176 - 20 * i);
    tft.printf("%3d", baseMinTemp + tempStep * i);
  }

  tft.setCursor(0, 66);
  tft.printf(" %cC", degreeSymbol);

  for (int i = 0; i < 12; i++) {
    if (i != 11) {
      tft.drawLine(23 * i + 30, mapf(temperature[i], baseMinTemp, baseMaxTemp, 180, 80),
                   23 * (i + 1) + 30, mapf(temperature[i + 1], baseMinTemp, baseMaxTemp, 180, 80),
                   ILI9341_RED);
    }
    tft.fillCircle(23 * i + 30, mapf(temperature[i], baseMinTemp, baseMaxTemp, 180, 80), 3, ILI9341_RED);
  }
}

void drawTemperatureText(float temperature[]) {
  float minTemp, maxTemp;
  char s[32];
  int16_t x, y;
  uint16_t w, h;

  getMinMax(temperature, 12, &minTemp, &maxTemp);

  tft.fillRect(20, 202, 280, 16, ILI9341_BLACK);
  tft.drawBitmap(10, 202, kanji[0], 16, 16, ILI9341_WHITE, ILI9341_BLACK);
  tft.drawBitmap(26, 202, kanji[1], 16, 16, ILI9341_WHITE, ILI9341_BLACK);
  sprintf(s, ": %5.1f C/ %5.1f C/ %5.1f C", temperature[0], minTemp, maxTemp);
  tft.setFont(&FreeSans9pt7b);
  tft.setCursor(48, 216);
  tft.print(s);

  for (int i = 0; i < 3; i++) {
    int n[] = {25, 17, 8};
    s[n[i]] = 0;
    tft.getTextBounds(s, 48, 210, &x, &y, &w, &h);
    tft.drawCircle(x + w + 3, 206, 2, ILI9341_WHITE);
  }
  tft.setFont();
}

void drawHumidityGraph(float humidity[]) {
  for (int i = 0; i < 12; i++) {
    if (i != 11) {
      tft.drawLine(23 * i + 36, mapf(humidity[i], 0, 100, 180, 80),
                   23 * (i + 1) + 36, mapf(humidity[i + 1], 0, 100, 180, 80),
                   ILI9341_BLUE);
    }
    tft.fillCircle(23 * i + 36, mapf(humidity[i], 0, 100, 180, 80), 3, ILI9341_BLUE);
  }
}

void drawWindSpeedText(float windSpeed[]) {
  float minSpeed, maxSpeed;

  getMinMax(windSpeed, 12, &minSpeed, &maxSpeed);

  tft.fillRect(20, 202, 280, 16, ILI9341_BLACK);
  tft.drawBitmap(10, 202, kanji[6], 16, 16, ILI9341_WHITE, ILI9341_BLACK);
  tft.drawBitmap(26, 202, kanji[7], 16, 16, ILI9341_WHITE, ILI9341_BLACK);
  tft.setFont(&FreeSans9pt7b);
  tft.setCursor(48, 216);
  tft.printf(": %3.1f m/s / %3.1f m/s / %3.1fm/s", windSpeed[0], minSpeed, maxSpeed);
  tft.setFont();
}

/* display routine */
void printInfo(void *arg) {
  int weather[13], precipProbability[13];
  float temperature[13], humidity[13], windSpeed[13];
  struct tm timeInfo;
  time_t lastUpdate = 0;
  int lastHour = -1;
  int needRedrawDisplay = 0, dataIsValid = 0;
  int currentText = -1;

  while (1) {
    time_t currentTime = time(NULL);
    localtime_r(&currentTime, &timeInfo);

    /* hour changed. shift data. */
    if ((dsParser.lastUpdate != 0) && (lastHour != timeInfo.tm_hour)) {
      for (int i = 0; i < 12; i++) {
        weather[i] = weather[i + 1];
        precipProbability[i] = precipProbability[i + 1];
        temperature[i] = temperature[i + 1];
        humidity[i] = humidity[i + 1];
        windSpeed[i] = windSpeed[i + 1];
      }
      lastHour = timeInfo.tm_hour;
      needRedrawDisplay = 1;
    }

    /* check if the data is valid */
    if ((dsParser.lastUpdate == 0) || ((currentTime - dsParser.lastUpdate) > 3600)) {
      /* no data or data is too old */
      for (int i = 0; i < 13; i++) {
        weather[i] = UNAVAILABLE;
      }
      needRedrawDisplay = 1;
      dataIsValid = 0;
    } else if (lastUpdate < dsParser.lastUpdate) {
      /* dark sky data has been updated */
      for (int i = 0, pos = 0; i < 14; i++) {
        if (i == 1) { /* skip dsParser.weatherData[1]. it is forecast data of current hour so abandon it. */
          continue;
        }
        /* set weather temperature humidity and precipitation probablity */
        weather[pos] =
          darkskyWeatherToIcon(dsParser.weatherData[i].weather, dsParser.weatherData[i].precipIntensity);
        temperature[pos] = dsParser.weatherData[i].temperature;
        humidity[pos] = dsParser.weatherData[i].humidity;
        precipProbability[pos] = dsParser.weatherData[i].precipProbability;
        windSpeed[pos] = dsParser.weatherData[i].windSpeed;
        pos++;
      }
      needRedrawDisplay = 1;
      dataIsValid = 1;
      lastUpdate = dsParser.lastUpdate;
    }

    if (needRedrawDisplay) {
      /* update hour */
      for (int i = 0; i < 12; i++) {
        tft.setCursor(23 * i + 29, 35);
        tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
        tft.printf("%2d", (timeInfo.tm_hour + i) % 24);
      }
      drawWeatherIcon(weather);

      if (dataIsValid) {
        tft.fillRect(22, 70, 276, 120, ILI9341_BLACK);
        drawGraph();
        drawPrecipProbabilityGraph(precipProbability);
        drawTemperatureGraph(temperature);
        drawHumidityGraph(humidity);
      } else {
        tft.fillRect(0, 76, 320, 106, ILI9341_BLACK);
      }
      needRedrawDisplay = 0;
    }

    /* update time */
    tft.setCursor(0, 15);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.printf("%04d/%02d/%02d %02d:%02d:%02d", timeInfo.tm_year + 1900,  timeInfo.tm_mon + 1, timeInfo.tm_mday,
               timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);

    if (dataIsValid) {
      if (((timeInfo.tm_sec / 10) % 3) == 0) {
        if (currentText != 0) {
          drawPrecipProbabilityText(precipProbability);
          currentText = 0;
        }
      } else if (((timeInfo.tm_sec / 10) % 3) == 1)  {
        if (currentText != 1) {
          drawTemperatureText(temperature);
          currentText = 1;
        }
      } else {
        if (currentText != 2) {
          drawWindSpeedText(windSpeed);
          currentText = 2;
        }
      }
    }
    delay(100);
  }
}

void setup() {
  Serial.begin(115200);

  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(200, 0);
  tft.setTextColor(ILI9341_BLUE, ILI9341_BLACK);
  tft.printf("Powered by Dark Sky");

  xTaskCreatePinnedToCore(printInfo, "printInfo", 4096, NULL, 1, NULL, 0);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  dsParser.begin(apiKey, latitude, longitude);
  configTzTime(tz, ntpServer1, ntpServer2, ntpServer3);
}

void loop() {
  dsParser.getData();
  Serial.printf("hour = %d\n", dsParser.currentHour);
  for (int i = 0; i < 14; i++) {
    Serial.printf("%02d: w = %2d, t = %4.1fC, h = %4.1f%%, w = %4.1fm/s, p = %4d%%, r = %4.1fmm\n",
                  i,
                  dsParser.weatherData[i].weather,
                  dsParser.weatherData[i].temperature,
                  dsParser.weatherData[i].humidity,
                  dsParser.weatherData[i].windSpeed,
                  dsParser.weatherData[i].precipProbability,
                  dsParser.weatherData[i].precipIntensity);
  }
  Serial.printf("Free Heap = %d\n", ESP.getFreeHeap());
  delay(((300 - (time(NULL) % 300)) + 10) * 1000);
}
