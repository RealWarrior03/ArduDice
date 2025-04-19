#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <ezButton.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define DICE_LIMIT 5

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define CLK_PIN 2
#define DT_PIN 3
#define SW_PIN 4
int CLK_state;
int prev_CLK_state;
//ezButton button(SW_PIN);
bool prevButtonPressed = HIGH;
volatile unsigned long last_time;

#define LINE_1 0
#define LINE_2 9
#define LINE_3 24

enum diceOS {
  LAUNCH,
  DICE_TYPE,
  DICE_COUNT,
  ROLLING,
  RESULT
};

int results[DICE_LIMIT] = {-1, -1, -1, -1, -1};
int currentDie = 0;
int total = 0;

const int DICE_TYPES[] = {4, 6, 8, 10, 12, 20};
const int NUM_DICE_TYPES = 6;

diceOS currentState = LAUNCH;
volatile int selectedDiceTypeIndex = 0;
int previousDiceTypeIndex = 0;
volatile int selectedDiceCount = 1;
int previousDiceCount = 1;


void setup() {
  //Serial.begin(115200);
  Serial.begin(9600);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  // configure encoder pins as inputs
  pinMode(CLK_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);
  //button.setDebounceTime(50);  // set debounce time to 50 milliseconds
  //attachInterrupt(digitalPinToInterrupt(CLK_PIN), updateEncoder, RISING);
  attachInterrupt(digitalPinToInterrupt(CLK_PIN), updateEncoder, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(DT_PIN), updateEncoder, CHANGE);
  // read the initial state of the rotary encoder's CLK pin
  prev_CLK_state = digitalRead(CLK_PIN);

  randomSeed(analogRead(0));

  launchScreen();
  display.display();
}

void loop() {
  // put your main code here, to run repeatedly:
  //button.loop();
  handleButtons();
  updateDisplay();
}

void handleButtons() {
  bool buttonPressed = digitalRead(SW_PIN);

  if (buttonPressed == LOW && prevButtonPressed == HIGH) {
    switch (currentState) {
      case LAUNCH:
        currentState = DICE_TYPE;
        break;
      case DICE_TYPE:
        currentState = DICE_COUNT;
        break;
      case DICE_COUNT:
        currentState = ROLLING;
        break;
      case ROLLING:
        currentState = RESULT;
        break;
      case RESULT:
        currentState = LAUNCH;
        break;
    }
  }

  prevButtonPressed = buttonPressed;
}

void rollDice() {
  while (currentDie < selectedDiceCount) {
    // Animation for remaining dice
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(selectedDiceCount);
    display.print("d");
    display.print(DICE_TYPES[selectedDiceTypeIndex]);
    display.print(" - Die ");
    display.print(currentDie + 1);
    display.print("/");
    display.println(selectedDiceCount);

    // Display dice and separators
    display.setTextSize(2);
    int xPos = 0;

    for (int i = 0; i < selectedDiceCount; i++) {
      display.setCursor(xPos, LINE_2);

      if (results[i] != -1) {
        // Show fixed result for already rolled dice
        display.print(results[i]);
        xPos += (results[i] < 10 ? 12 : 18);
      } else {
        // Show animation for unrolled dice
        int animResult = random(1, DICE_TYPES[selectedDiceTypeIndex] + 1);
        display.print(animResult);
        xPos += (animResult < 10 ? 12 : 18);
      }

      // Add separator after all but the last dice
      if (i < selectedDiceCount - 1) {
        display.setCursor(xPos, LINE_2);
        display.print("|");
        xPos += (results[i] < 10 ? 12 : 18);
      }
    }

    // Show running total
    if (currentDie > 0) {
      display.setTextSize(1);
      display.setCursor(0, LINE_3);
      display.print("Running total: ");
      display.print(total);
    }

    display.display();
    //delay(100); //Animation speed

    bool buttonPressed = digitalRead(SW_PIN);
    if (buttonPressed == LOW && prevButtonPressed == HIGH) {
      // Fix the current die result
      results[currentDie] = random(1, DICE_TYPES[selectedDiceTypeIndex] + 1);
      total += results[currentDie];
      currentDie++;

      // If this was the last die, show final results
      if (currentDie == selectedDiceCount) {
        delay(200);
        currentState = RESULT;
        break;
      }

      delay(200);
    }
    prevButtonPressed = buttonPressed;
  }
}

void showResults(int total, int results[]) {
  bool showResults = true;
  while (showResults) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, LINE_1);
    display.print(selectedDiceCount);
    display.print("d");
    display.print(DICE_TYPES[selectedDiceTypeIndex]);
    if (selectedDiceCount > 1) {
      display.print(" - Total: ");
      display.print(total);
    }

    //Display specific results
    display.setTextSize(2);
    int finalXPos = 0;

    for (int i = 0; i < selectedDiceCount; i++) {
      display.setCursor(finalXPos, LINE_2);
      display.print(results[i]);
      finalXPos += (results[i] < 10 ? 12 : 18);

      if (i < selectedDiceCount - 1) {
        display.setCursor(finalXPos, LINE_2);
        display.print("|");
        finalXPos += (results[i] < 10 ? 12 : 18);
      }
    }

    display.display();

    bool buttonPressed = digitalRead(SW_PIN);
    if (buttonPressed == LOW && prevButtonPressed == HIGH) {
      total = 0;
      for (int i = 0; i < DICE_LIMIT; i++) {
        results[i] = -1;
      }
      currentDie = 0;
      currentState = LAUNCH;
      showResults = false;
      delay(200);
      break;
    }
    prevButtonPressed = buttonPressed;
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(WHITE);

  switch (currentState) {
    case LAUNCH:
      launchScreen();
      break;
    case DICE_TYPE:
      displayDiceTypeMenu();
      break;
    case DICE_COUNT:
      displayDiceCountMenu();
      break;
    case ROLLING:
      rollDice();
      break;
    case RESULT:
      showResults(total, results);
      break;
  }
  
  display.display();
}

void displayDiceTypeMenu() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Select Dice Type:");

  display.setTextSize(2);
  display.setCursor(30, 16);
  display.print("d");
  display.println(DICE_TYPES[selectedDiceTypeIndex]);
}

void displayDiceCountMenu() {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Number of Dice:");
  
  display.setTextSize(2);
  display.setCursor(30, 16);
  display.println(selectedDiceCount);
}

void launchScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("ArduDice");
  display.setTextSize(1);
  display.setCursor(0, 24);
  display.println("Press any button!");
}

void updateEncoder() {
  if ((millis() - last_time) < 50)  // debounce time is 50ms
    return;

  if (digitalRead(DT_PIN) == LOW) {
    // the encoder is rotating in counter-clockwise direction => decrease the counter
    switch (currentState) {
      case DICE_TYPE:
        selectedDiceTypeIndex = (selectedDiceTypeIndex - 1 + NUM_DICE_TYPES) % NUM_DICE_TYPES;
        break;
      case DICE_COUNT:
        selectedDiceCount = max(selectedDiceCount - 1, 1);
        break;
    }
  } else {
    // the encoder is rotating in clockwise direction => increase the counter
    switch (currentState) {
      case DICE_TYPE:
        selectedDiceTypeIndex = (selectedDiceTypeIndex + 1) % NUM_DICE_TYPES;
        break;
      case DICE_COUNT:
        selectedDiceCount = min(selectedDiceCount + 1, DICE_LIMIT);
        break;
    }
  }

  last_time = millis();
}
