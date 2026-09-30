# 26048_SHADOW_VAULT

Problem Statement :
Kwatha is usually made by hand, and it is easy to get wrong:
* The water-to-herb ratio is measured by eye
* The heat goes up and down, which spoils the quality
* The result does not meet clinical standards
* Kwatha goes bad within 24 hours, so it must be made fresh every time
This affects city homes that need kadha every day, AYUSH clinics, and elderly patients on prescribed kwatha. Many of them only have loose powder, and many cannot read a menu in English.

Proposed Solution :
iKwath measures each step instead of relying on the user's judgment. It uses weighed dosing, gentle indirect heat, a sealed loop that pushes water through the herb bed, and a measured end point.
The basic operation is:
WEIGH -> HEAT -> RECIRCULATE -> MEASURE -> SERVE -> RECORD
The machine doses the water by weight, holds it at 85 to 90 °C, circulates it through the herb, stops when the dissolved solids stop rising, and logs the batch.

---

# iKwath Display Subsystem & UI Architecture

This repository contains the ESP32 Display Controller and User Interface mockup for the iKwath System.

## Contents
- **`index.html`**: Interactive web-based simulator of the 2.8" TFT UI, featuring soft translucent infrared heat spot overlays for pain localization.
- **`iKwath_Display_UI.ino`**: ESP32 firmware source code for the ILI9341 TFT display and rotary encoder controls.
- **`ikwath_ui_specification.md`**: Architectural and backend specification document detailing parameters like water dosing and heater PID bands.
- **Assets**: High-definition athletic male model photos for visual symptom presentation (`masculine_body_pain...`, `cold_chest_pain...`, `headache_pain...`).

## Live UI Demo
*(Add your GitHub Pages link here once deployed: `https://<your-username>.github.io/<repo-name>`)*

## How to Deploy the Live Link via GitHub Pages
1. In your GitHub repository, go to **Settings** > **Pages**.
2. Under "Build and deployment", set the **Source** to `Deploy from a branch`.
3. Select the `master` or `main` branch and `/ (root)` folder, then click **Save**.
4. Wait a minute or two, and GitHub will provide you with a live link to your `index.html` simulator! You can then add this link to your repository's About section.
