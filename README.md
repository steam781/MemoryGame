# 8x8 Matrix Memory Game ⏳🧠

Detta är mitt slutprojekt i kursen Enchipsdatorer på Jönköping University (JTH). Spelet utmanar spelarens visuella minne och snabbhet på en 8x8 LED-matris. Ett slumpmässigt mönster visas kort, och spelaren måste återskapa så mycket som möjligt innan tiden rinner ut. Efteråt får man visuell feedback och en animerad poängräkning.

## 🛠️ Hårdvara och Komponenter
* 1x STM32 Nucleo-64 (NUCLEO-L433RC-P)
* 1x 8x8 LED-matris med MAX7219-drivrutin
* 1x Analog Joystick (för X- och Y-navigering)
* 1x Tryckknapp (för "Välj/Bekräfta")
* Kopplingsdäck (Breadboard) och kablar

## ⚡ Kopplingsschema (Pinout)
Systemet kommunicerar med skärmen via SPI1 och läser in joystickens positioner via mikrokontrollerns inbyggda ADC.

| Komponent | Pin (MCU) | Funktion / Typ |
| :--- | :--- | :--- |
| **Matris DIN (Data)** | SPI1 MOSI | SPI Data Out |
| **Matris CS (Load)** | GPIOA (`MTRX_CS_Pin`) | Mjukvarustyrd Chip Select |
| **Matris CLK (Clock)** | SPI1 SCK | SPI Clock |
| **Joystick Y-axel** | ADC1 Channel 1 | Analog In (ADC Rank 1) |
| **Joystick X-axel** | ADC1 Channel 2 | Analog In (ADC Rank 2) |
| **Knapp: Välj** | PC2 (`SWITCH_BUTTON_Pin`) | Digital In (Aktiv Låg) |

<img width="870" height="772" alt="image" src="https://github.com/user-attachments/assets/096d9a96-d47a-478f-97de-ee8ba915a9d9" />

## 🎮 Hur man spelar
1. **Memorera:** Skärmen visar ett helt slumpmässigt genererat mönster.
2. **Spela mot klockan:** Så fort du startar har du exakt 30 sekunder på dig. Använd joysticken för att navigera markören och tryck på PC2-knappen för att välja LEDs.
3. **Feedback:** När de 30 sekunderna löper ut fryser spelet och rensar bort alla felaktiga knapptryckningar så att du kan se vilka rätt du hade.
4. **Poängberäkning:** 
   * **+1 poäng** för varje rätt.
   * **-0,5 poäng** för varje fel (spelet tillåter inga minuspoäng totalt).
5. **Resultatvisning:** Spelet animerar fram din totala poäng på displayen med en egenutvecklad 3x5-pixels font!

## 🧠 Systemarkitektur & Tekniker under huven
Koden demonstrerar flera viktiga koncept för bare-metal- och HAL-programmering på STM32:

* **Tidshantering & Interrupts (TIM2):** `TIM2` genererar ett interrupt varje millisekund (`tick++`). Detta fungerar som systemets ryggrad och hanterar 30-sekundersgränsen, blinkande grafik, ADC-polling (var 50:e ms) och uppdatering av skärmen (50 Hz) – allt utan blockerande `HAL_Delay()` i main-loopen under spelets gång.
* **Hårdvaru-RNG:** Istället för standard C-kod (`rand()`) används STM32:ans inbyggda hårdvaru-RNG (`HAL_RNG_GenerateRandomNumber`) för att få ett genuint slumpmässigt brus (True Random) som seedar varje runda.
* **Displayhantering & Framebuffer:** En 2D-array (`myMatrix`) fungerar som framebuffer. Spelet uppdaterar markörens position i minnet (XOR-logik för blinkeffekt) innan hela arrayen översätts till bits och pushas ut till MAX7219 via `SPI1`.
* **ADC med Multi-Channel Polling:** Joysticken läses kontinuerligt av med `ADC1` konfigurerad för scan mode med två ranker. `THRESHOLD`-makron skapar inbyggda deadzones för att förhindra drift.
* **Effektiv Matematik:** Poängsystemet, som innehåller halva minuspoäng, hanteras helt och hållet med integer-matematik (`temp_score - (temp_wrong/2)`). Detta sparar CPU-cykler och minne genom att helt undvika flyttal (floats).
* **Tillståndsmaskin:** Hela programflödet struktureras i en State Machine (`STATE_MENU`, `STATE_PLAYING`, `STATE_GAME_OVER`).

## 🚀 Kompilering & Installation
1. Klona repot: `git clone https://github.com/steam781/MemoryGame.git`
2. Öppna projektet i STM32CubeIDE.
3. Kompilera för [din specifika MCU].
4. Flasha koden till kortet.
