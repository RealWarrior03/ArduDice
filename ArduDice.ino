#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define DICE_LIMIT 5

#define LINE_1 0
#define LINE_2 9
#define LINE_3 24

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int button1 = 4;
const int button2 = 3;
const int button3 = 2;

bool lastButton1State = HIGH;
bool lastButton2State = HIGH;
bool lastButton3State = HIGH;

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
int selectedDiceTypeIndex = 0;
int selectedDiceCount = 1;


void setup() {
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(button3, INPUT_PULLUP);

  Serial.begin(115200);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  randomSeed(analogRead(0));

  launchScreen();
  display.display();
}

void loop() {
  // put your main code here, to run repeatedly:
  handleButtons();
  updateDisplay();
  delay(50);
}

void handleButtons() {
  bool button1State = digitalRead(button1);
  bool button2State = digitalRead(button2);
  bool button3State = digitalRead(button3);

  // Button 1: Up/Increase
  if (button1State == LOW && lastButton1State == HIGH) {
    switch (currentState) {
      case LAUNCH:
        currentState = DICE_TYPE;
        break;
      case DICE_TYPE:
        selectedDiceTypeIndex = (selectedDiceTypeIndex + 1) % NUM_DICE_TYPES;
        break;
      case DICE_COUNT:
        selectedDiceCount = min(selectedDiceCount + 1, DICE_LIMIT);
        break;
    }
  }

  // Button 2: Down/Decrease
  if (button2State == LOW && lastButton2State == HIGH) {
    switch (currentState) {
      case LAUNCH:
        currentState = DICE_TYPE;
        break;
      case DICE_TYPE:
        selectedDiceTypeIndex = (selectedDiceTypeIndex - 1 + NUM_DICE_TYPES) % NUM_DICE_TYPES;
        break;
      case DICE_COUNT:
        selectedDiceCount = max(selectedDiceCount - 1, 1);
        break;
    }
  }

  // Button 3: Confirm/Roll
  if (button3State == LOW && lastButton3State == HIGH) {
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

  lastButton1State = button1State;
  lastButton2State = button2State;
  lastButton3State = button3State;
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

    // Check for button press to fix current die
    bool button3State = digitalRead(button3);
    if (button3State == LOW && lastButton3State == HIGH) {
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
    lastButton3State = button3State;
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

    bool button3State = digitalRead(button3);
    if (button3State == LOW && lastButton3State == HIGH) {
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
    lastButton3State = button3State;
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
