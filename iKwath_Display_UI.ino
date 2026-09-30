/*
 * Project: iKwath Smart Kwatha Maker
 * File: main_display_controller.ino
 * Description: 2.8-inch TFT LCD UI driver for ESP32.
 *              Configured for 3 prescribed kwatha options with soft translucent red pain heat aura overlays.
 *              Clean patient UI (Ailment, Powder, Dose). Engineering metrics (Temp, TDS, Water)
 *              are processed and logged in the backend.
 * Hardware: ESP32-WROOM-32 + 2.8" ILI9341 SPI TFT (320x240) + Rotary Encoder
 *
 * Pin Connections:
 *   TFT_CS    -> GPIO 15
 *   TFT_DC    -> GPIO 2
 *   TFT_RST   -> GPIO 4
 *   TFT_MOSI  -> GPIO 23
 *   TFT_SCLK  -> GPIO 18
 *   TFT_BL    -> GPIO 32
 *   ROT_CLK   -> GPIO 25 (Interrupt)
 *   ROT_DT    -> GPIO 26
 *   ROT_SW    -> GPIO 27
 */

#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

#define PIN_ROTARY_CLK  25
#define PIN_ROTARY_DT   26
#define PIN_ROTARY_SW   27
#define PIN_TFT_BL      32

// Color Constants (16-bit RGB565)
#define COLOR_BG          0x0821  // Dark Charcoal (#0b0f19)
#define COLOR_PANEL       0x18E5  // Panel Blue-Grey (#1e293b)
#define COLOR_BORDER      0x31A6  // Border Slate (#334155)
#define COLOR_TEXT_WHITE  0xFFFF  // Text Primary
#define COLOR_TEXT_MUTED  0x9E3B  // Text Secondary
#define COLOR_EMERALD     0x1ED1  // Accent Green (#10b981)
#define COLOR_AMBER       0xFCC0  // Accent Amber (#f59e0b)
#define COLOR_CYAN        0x07FF  // Accent Cyan (#06b6d4)
#define COLOR_RED_PAIN    0xF984  // Translucent Soft Coral Red (Decreased darkness/opacity)
#define COLOR_RED_GLOW    0xFA48  // Soft Light Red Aura (#ff8080)

struct AilmentRecipe {
  const char* title;         // Displayed to Patient
  const char* powderName;    // Displayed to Patient
  const char* doseWeight;    // Displayed to Patient ("21.0 g")
  uint16_t targetWaterMl;    // Backend Dosing Parameter (84 ml)
  float targetTempMin;       // Backend PID Parameter (85.0 C)
  float targetTempMax;       // Backend PID Parameter (90.0 C)
  uint16_t targetDeltaTDS;   // Backend End-point Target (312 ppm)
  const char* zoneLabel;     // Pain location tag
  uint16_t painX;            // X coordinate on 148px photo pane
  uint16_t painY;            // Y coordinate on 192px photo pane
  uint8_t  painRadius;       // Red spot radius
};

// 3 Prescribed Kwatha Recipes matching the 3 Photo Assets
const AilmentRecipe recipes[3] = {
  { "Joint & Knee Pain", "Dashamula Yavakuta", "21.0 g", 84, 85.0, 90.0, 312, "Knee & Joint Pain", 52, 162, 10 },
  { "Cold & Chest Congest", "Trikatu & Sitopaladi", "21.0 g", 84, 88.0, 90.0, 285, "Chest & Lungs Area", 52, 65, 12 },
  { "Headache & Migraine", "Pathyadi & Jatamansi", "21.0 g", 84, 85.0, 88.0, 260, "Forehead & Temples", 52, 38, 8 }
};

volatile int activeIndex = 0; // Menu index (0 to 2)
volatile bool screenUpdateNeeded = true;
int previousClkState;
bool isBackendDiagnosticMode = false;

// Prototypes
void renderTopBar();
void renderDiseasePhotoPane(int index);
void renderMinimalPatientPanel(int index);
void renderBackendDiagnosticsScreen(int index);
void renderFooter();
void IRAM_ATTR handleRotaryEncoderISR();

void setup() {
  Serial.begin(115200);

  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);

  pinMode(PIN_ROTARY_CLK, INPUT_PULLUP);
  pinMode(PIN_ROTARY_DT, INPUT_PULLUP);
  pinMode(PIN_ROTARY_SW, INPUT_PULLUP);
  previousClkState = digitalRead(PIN_ROTARY_CLK);

  attachInterrupt(digitalPinToInterrupt(PIN_ROTARY_CLK), handleRotaryEncoderISR, CHANGE);

  tft.init();
  tft.setRotation(1); // 320x240 Landscape
  tft.fillScreen(COLOR_BG);

  Serial.println(F("[System] iKwath Display Controller Online (Soft Opacity Red Aura)."));
}

void loop() {
  if (digitalRead(PIN_ROTARY_SW) == LOW) {
    unsigned long pressStart = millis();
    while (digitalRead(PIN_ROTARY_SW) == LOW) {
      delay(10);
    }
    unsigned long pressDuration = millis() - pressStart;

    if (pressDuration > 1200) {
      isBackendDiagnosticMode = !isBackendDiagnosticMode;
      screenUpdateNeeded = true;
    } else if (pressDuration > 40) {
      tft.fillRect(80, 80, 230, 75, COLOR_AMBER);
      tft.drawRect(80, 80, 230, 75, COLOR_TEXT_WHITE);
      tft.setTextColor(COLOR_BG, COLOR_AMBER);
      tft.drawString("STARTING BREW CYCLE...", 92, 95, 2);
      tft.drawString(recipes[activeIndex].powderName, 92, 120, 1);
      
      Serial.printf("[Backend Brew] Ailment: %s | Powder: %s | Dose: %s\n",
                    recipes[activeIndex].title, recipes[activeIndex].powderName, recipes[activeIndex].doseWeight);
                    
      delay(2000);
      screenUpdateNeeded = true;
    }
  }

  if (screenUpdateNeeded) {
    screenUpdateNeeded = false;
    tft.fillScreen(COLOR_BG);
    
    if (!isBackendDiagnosticMode) {
      renderTopBar();
      renderDiseasePhotoPane(activeIndex);
      renderMinimalPatientPanel(activeIndex);
      renderFooter();
    } else {
      renderBackendDiagnosticsScreen(activeIndex);
    }
  }

  delay(10);
}

void renderTopBar() {
  tft.fillRect(0, 0, 320, 22, COLOR_PANEL);
  tft.drawFastHLine(0, 22, 320, COLOR_BORDER);

  tft.setTextColor(COLOR_AMBER, COLOR_PANEL);
  tft.drawString("iKwath System", 6, 4, 2);

  char navBuf[16];
  snprintf(navBuf, sizeof(navBuf), "%d / 3", activeIndex + 1);
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL);
  tft.drawString(navBuf, 150, 6, 1);

  tft.setTextColor(COLOR_EMERALD, COLOR_PANEL);
  tft.drawString("READY", 270, 6, 1);
}

// Renders Elongated Left Photo Pane (50% Screen Width: 148px) with Soft Translucent Red Pain Aura
void renderDiseasePhotoPane(int idx) {
  const AilmentRecipe &item = recipes[idx];

  tft.fillRect(6, 28, 148, 192, COLOR_PANEL);
  tft.drawRect(6, 28, 148, 192, COLOR_BORDER);

  tft.fillRect(8, 30, 144, 16, COLOR_RED_PAIN);
  tft.setTextColor(COLOR_TEXT_WHITE, COLOR_RED_PAIN);
  tft.drawCentreString("TARGET PAIN PHOTO", 80, 32, 1);

  // Dynamic Soft Red Heat Spot Overlay (Lower Darkness / Soft Translucent Red)
  uint16_t cx = 8, cy = 28;
  tft.drawCircle(cx + item.painX, cy + item.painY, item.painRadius + 6, COLOR_RED_GLOW);
  tft.fillCircle(cx + item.painX, cy + item.painY, item.painRadius + 2, COLOR_RED_GLOW);
  tft.fillCircle(cx + item.painX, cy + item.painY, item.painRadius - 2, COLOR_RED_PAIN);
  tft.fillCircle(cx + item.painX, cy + item.painY, 3, COLOR_TEXT_WHITE);

  tft.fillRect(8, 196, 144, 20, COLOR_RED_PAIN);
  tft.setTextColor(COLOR_TEXT_WHITE, COLOR_RED_PAIN);
  tft.drawCentreString(item.zoneLabel, 80, 200, 1);
}

void renderMinimalPatientPanel(int idx) {
  const AilmentRecipe &item = recipes[idx];

  tft.fillRect(160, 28, 154, 36, COLOR_PANEL);
  tft.drawRect(160, 28, 154, 36, COLOR_RED_PAIN);
  
  tft.setTextColor(COLOR_RED_GLOW, COLOR_PANEL);
  tft.drawString("PRESCRIPTION", 166, 32, 1);
  tft.setTextColor(COLOR_TEXT_WHITE, COLOR_PANEL);
  tft.drawString(item.title, 166, 44, 2);

  tft.fillRect(160, 70, 154, 118, COLOR_PANEL);
  tft.drawRect(160, 70, 154, 118, COLOR_BORDER);

  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL);
  tft.drawString("Herbal Powder:", 166, 80, 1);
  tft.setTextColor(COLOR_AMBER, COLOR_PANEL);
  tft.drawString(item.powderName, 166, 96, 1);

  tft.drawFastHLine(166, 122, 142, COLOR_BORDER);

  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL);
  tft.drawString("Dose Weight:", 166, 134, 1);
  tft.setTextColor(COLOR_EMERALD, COLOR_PANEL);
  tft.drawString(item.doseWeight, 166, 148, 2);

  tft.fillRect(160, 198, 154, 22, COLOR_EMERALD);
  tft.setTextColor(COLOR_BG, COLOR_EMERALD);
  tft.drawCentreString("PRESS TO BREW", 237, 203, 1);
}

void renderBackendDiagnosticsScreen(int idx) {
  const AilmentRecipe &item = recipes[idx];

  tft.fillRect(0, 0, 320, 240, COLOR_BG);
  tft.fillRect(0, 0, 320, 22, COLOR_PANEL);
  tft.setTextColor(COLOR_AMBER, COLOR_PANEL);
  tft.drawString("[BACKEND DIAGNOSTICS LOG]", 6, 4, 2);

  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
  tft.drawString("Ailment:", 10, 32, 1);
  tft.setTextColor(COLOR_TEXT_WHITE, COLOR_BG);
  tft.drawString(item.title, 90, 32, 1);

  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
  tft.drawString("Powder Formulation:", 10, 48, 1);
  tft.setTextColor(COLOR_AMBER, COLOR_BG);
  tft.drawString(item.powderName, 135, 48, 1);

  tft.drawFastHLine(10, 65, 300, COLOR_BORDER);

  tft.setTextColor(COLOR_CYAN, COLOR_BG);
  tft.drawString("Water Weight Dosing Target:", 10, 75, 1);
  tft.setTextColor(COLOR_TEXT_WHITE, COLOR_BG);
  char buf[32];
  snprintf(buf, sizeof(buf), "%d ml (Load Cell HX711)", item.targetWaterMl);
  tft.drawString(buf, 175, 75, 1);

  tft.setTextColor(COLOR_CYAN, COLOR_BG);
  tft.drawString("Water Bath Heater PID:", 10, 95, 1);
  tft.setTextColor(COLOR_AMBER, COLOR_BG);
  snprintf(buf, sizeof(buf), "%.1f C - %.1f C Band", item.targetTempMin, item.targetTempMax);
  tft.drawString(buf, 160, 95, 1);

  tft.setTextColor(COLOR_CYAN, COLOR_BG);
  tft.drawString("Target Delta TDS:", 10, 115, 1);
  tft.setTextColor(COLOR_EMERALD, COLOR_BG);
  snprintf(buf, sizeof(buf), "%d ppm (End-Point Control)", item.targetDeltaTDS);
  tft.drawString(buf, 130, 115, 1);

  tft.drawFastHLine(10, 135, 300, COLOR_BORDER);

  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
  tft.drawString("Safety Systems:", 10, 145, 1);
  tft.drawString("30mA RCD | Thermal Fuse 120C | Software Cutoff 95C", 10, 160, 1);
  tft.drawString("Cloud Logging: Firebase DB Active", 10, 175, 1);

  tft.fillRect(10, 200, 300, 26, COLOR_PANEL);
  tft.drawRect(10, 200, 300, 26, COLOR_BORDER);
  tft.setTextColor(COLOR_TEXT_WHITE, COLOR_PANEL);
  tft.drawCentreString("Hold Knob >1.2s to Exit Diagnostics", 160, 206, 1);
}

void renderFooter() {
  tft.fillRect(0, 224, 320, 16, COLOR_BG);
  tft.drawFastHLine(0, 224, 320, COLOR_BORDER);
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
  tft.drawString("Vaidya prescribed. Not an automated diagnosis.", 6, 228, 1);
}

void IRAM_ATTR handleRotaryEncoderISR() {
  int currentClk = digitalRead(PIN_ROTARY_CLK);
  if (currentClk != previousClkState && currentClk == LOW) {
    if (digitalRead(PIN_ROTARY_DT) != currentClk) {
      activeIndex = (activeIndex + 1) % 3;
    } else {
      activeIndex = (activeIndex - 1 + 3) % 3;
    }
    screenUpdateNeeded = true;
  }
  previousClkState = currentClk;
}
