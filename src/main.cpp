#include <Arduino.h>
#include <JC_Button.h>    // https://github.com/JChristensen/JC_Button
#include <Adafruit_GFX.h> // Core graphics library
#include <SPI.h>
#include "Adafruit_ST7789_Fri3d2024.h" // Hardware-specific library for ST7789 driver
#include <Preferences.h>

Preferences preferences;

// Joystick pins
#define PIN_JOY_X 1
#define PIN_JOY_Y 3
#define BUTTON_A 39 // Example pin for button A

// Initialize the display
SPIClass *spi = new SPIClass(HSPI);
Adafruit_GFX_Fri3dBadge2024_TFT tft(spi, TFT_CS, TFT_DC, TFT_RST);

// Initialize buttons
// Button(pin, debounceTime, pullUpEnable, invert)
Button button_A(39, 25, true, true);
Button button_B(40, 25, true, true);
Button button_X(38, 25, true, true);
Button button_Y(41, 25, true, true);
Button button_MENU(45, 25, true, true);
Button button_START(0, 25, false, true); // GPIO0 has HW fixed pullup

// Define grid dimensions for the letters
const int cols = 5;
const int rows = 6;
const char letters[rows][cols] = {
    {'A', 'B', 'C', 'D', 'E'},
    {'F', 'G', 'H', 'I', 'J'},
    {'K', 'L', 'M', 'N', 'O'},
    {'P', 'Q', 'R', 'S', 'T'},
    {'U', 'V', 'W', 'X', 'Y'},
    {'Z', ' ', '.', ',', '-'}};

String state = "setup";
int cursorX = 0;
int cursorY = 0;
int keyBoardYOffset = 60;

// beacuse the display is rotated
int screen_width = TFT_HEIGHT;
int screen_height = TFT_WIDTH;

//STABILITY DELAY
int stability_delay = 60;

String name = "";
int cellWidth = (screen_height / 2) / cols;
int cellHeight = ((screen_width / 2)) / rows; // Reserve 40 pixels for the name display
int keyboardWidth = cols * cellWidth;
// joystick state
boolean joystick_engaged_x = false;
boolean joystick_engaged_y = false;
// Animation
// String animation = "invert"; // "upDown" or "invert"
String animation = "upDown"; // "upDown" or "invert"
int animationY = 0;
int animationDirection = 1;
int animationSpeed = 1;

void displayCenteredText(String text, int centerX, int centerY, int textSize, boolean removeBackground = false, boolean maxSize = false, int color = TFT_WHITE)
{
    if (maxSize)
    {
        tft.setTextWrap(false);

        // Find the largest font size that fits the screen width
        int16_t x1, y1;
        uint16_t w, h;
        uint8_t font_size = 1;
        int16_t max_width = screen_width;
        do
        {
            tft.setTextSize(font_size);
            tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
            font_size++;
        } while (w < max_width && h < tft.height() - 20); // Also check height to avoid overflow
        // Step back one size to ensure it fits
        font_size -= 2;
        textSize = font_size;
    }
    tft.setTextSize(textSize);

    // Calculate width of the text
    int16_t x1, y1;
    uint16_t w, h;
    // check if text is empty, put space instead
    if (text == "")
    {
        text = " ";
    }
    tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h); // Calculate bounds, (0,0) is temporary

    // Calculate the start point (x, y)
    int startX = centerX - w / 2;
    int startY = centerY - h / 2;
    Serial.print(" w: ");
    Serial.print(w);
    Serial.print(" h: ");
    Serial.print(h);
    Serial.print(" x: ");
    Serial.print(startX);
    Serial.print(" y: ");
    Serial.print(startY);
    Serial.print(" centerX: ");
    Serial.print(centerX);
    Serial.print(" centerY: ");
    Serial.println(centerY);

    // Set the cursor to the calculated position
    if (removeBackground){
        tft.fillRect(0, startY, screen_width, h, TFT_BLACK); // Clear the previous text
    }
    tft.setCursor(startX, startY);
    tft.print(text);
}

void displayText(String text)
{
    // tft.setCursor(10, screen_width - 30);
    // tft.setCursor(screen_height/2, 0);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    // tft.println(text);
    Serial.println(text);
    Serial.print("screen width: ");
    Serial.println(screen_height);

    // tft.setCursor(240, 0);
    // tft.print(text);

    // height  reres
    // displayCenteredText(text, screen_width / 2, screen_height / 4, 3); // Display text centered at (x, y) with size 2
    displayCenteredText(text, screen_width / 2, 20, 3, true); // Display text centered at (x, y) with size 2
}

void nameTag(String text)
{
    Serial.println("Entering nameTag function");

    if (text.length() > 0)
    {
        Serial.println("Attempting to open preferences");
        if (preferences.begin("nametag-app", false))
        {
            Serial.println("Preferences opened successfully");
            Serial.println("Saving name to NVS");
            size_t written = preferences.putString("username", text);
            if (written == text.length())
            {
                Serial.println("Name saved successfully");
            }
            else
            {
                Serial.println("Failed to save name. Bytes written: " + String(written));
            }
            preferences.end();
            Serial.println("Preferences closed");
        }
        else
        {
            Serial.println("Failed to open preferences for writing");
        }
    }
    else
    {
        Serial.println("Not saving empty name to NVS");
    }

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    displayCenteredText(text, screen_width / 2, screen_height / 2, 4, false, true);

    Serial.println("Exiting nameTag function");
}
void drawLetter(int x, int y, char letter, int color = TFT_WHITE)
{
    int posX = x * cellWidth + (screen_width / 2 - keyboardWidth / 2);
    int posY = y * cellHeight;

    // int posX = x * cellWidth + ((screen_height/2) - (cols * cellWidth/2));
    // int posY = y * cellHeight + ((screen_width/2)- (rows * cellHeight/2));
    tft.setCursor(posX + cellWidth / 4, keyBoardYOffset + posY + cellHeight / 4);
    Serial.println(posX + cellWidth / 4);
    // tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextColor(color);
    tft.setTextSize(2);
    tft.print(letter);
}

void highlightCursor(int x, int y)
{
    int posX = x * cellWidth + (screen_width / 2 - keyboardWidth / 2);
    int posY = (y * cellHeight) + keyBoardYOffset;
    // int posX = x * cellWidth + ((screen_height / 2) - (cols * cellWidth / 2));
    // int posY = y * cellHeight + ((screen_width / 2) - (rows * cellHeight / 2));

    // tft.drawRect(posX, posY, cellWidth, cellHeight, TFT_RED);
    tft.fillRect(posX, posY, cellWidth, cellHeight, TFT_WHITE);
    drawLetter(x, y, letters[y][x], TFT_BLACK);
}
void removeHighlight(int x, int y)
{
    int posX = x * cellWidth + (screen_width / 2 - keyboardWidth / 2);
    int posY = (y * cellHeight) + keyBoardYOffset;
    // int posX = x * cellWidth + ((screen_height / 2) - (cols * cellWidth / 2));
    // int posY = y * cellHeight + ((screen_width / 2) - (rows * cellHeight / 2));
    // tft.drawRect(posX, posY, cellWidth, cellHeight, TFT_BLACK);
    tft.fillRect(posX, posY, cellWidth, cellHeight, TFT_BLACK);
    drawLetter(x, y, letters[y][x]);
}

void handleJoystick()
{
    int newx = map(analogRead(PIN_JOY_X), 0, 4096, -1, 2);
    int newy = map(analogRead(PIN_JOY_Y), 0, 4096, 1, -2);
    // Serial.print("X: ");
    // Serial.print(newx);
    // Serial.print(" Y: ");
    // Serial.println(newy);
    if (joystick_engaged_x && newx == 0)
    {
        joystick_engaged_x = false;
    }
    if (joystick_engaged_y && newy == 0)
    {
        joystick_engaged_y = false;
    }

    if (newx != 0 || newy != 0)
    {
        if (!joystick_engaged_x)
        {
            removeHighlight(cursorX, cursorY);
            cursorX = constrain(cursorX + newx, 0, cols - 1);
            joystick_engaged_x = true;

            highlightCursor(cursorX, cursorY);
        }
        if (!joystick_engaged_y)
        {
            removeHighlight(cursorX, cursorY);
            cursorY = constrain(cursorY + newy, 0, rows - 1);
            joystick_engaged_y = true;

            highlightCursor(cursorX, cursorY);
        }
        // cursorX = cursorX + newx;
        // cursorY = cursorY + newy;
        // cursorX = constrain(cursorX, 0, cols - 1);
        // cursorY = constrain(cursorY, 0, rows - 1);

    }
}
void drawGrid()
{
    for (int y = 0; y < rows; y++)
    {
        for (int x = 0; x < cols; x++)
        {
            drawLetter(x, y, letters[y][x]);
        }
    }
}

void handleButtons(const char *buttonname)
{
    if (strcmp(buttonname, "A") == 0)
    {
        name += letters[cursorY][cursorX];
        Serial.println(name);
        displayText(name);
    }
    if (strcmp(buttonname, "B") == 0)
    {
        name = name.substring(0, name.length() - 1);
        Serial.println(name);
        displayText(name);
    }
    if (strcmp(buttonname, "START") == 0)
    {
        state = "nametag";
        nameTag(name);
    }
    if (strcmp(buttonname, "MENU") == 0)
    {
        state = "setup";
        // displayText("Enter your name");
        // name = "";
        // cursorX = 0;
        // cursorY = 0;
        tft.fillScreen(TFT_BLACK);
        displayText(name);
        drawGrid();
        highlightCursor(cursorX, cursorY);
    }
}

void checkButton(Button *b, const char *buttonname)
{

    b->read();

    if (b->wasPressed())
    {
        Serial.print(buttonname);
        Serial.println(" pressed");
        handleButtons(buttonname);
    }
    if (b->wasReleased())
    {
        Serial.print(buttonname);
        Serial.println(" released");
    }
}

// void updateAnimation()
// {
//     if (animation == "upDown")
//     {


//     }
// }
void updateAnimation()
{
    if (animation == "upDown")
    {
        if (millis() % 100 < 50)
        {
            int displayHeight = screen_height / 2; // Central vertical position
            int movementRange = 10;                // Range of movement above and below the central position

            // Update the Y position of the animation based on the direction
            if (animationDirection == 1)
            {
                animationY += 1; // Move the text up
                if (animationY >= movementRange)
                {
                    animationDirection = -1; // Change direction to down
                }
            }
            else
            {
                animationY -= 1; // Move the text down
                if (animationY <= -movementRange)
                {
                    animationDirection = 1; // Change direction to up
                }
            }

            // Clear the exact previous position of the text to avoid flickering
            int16_t x1, y1;
            uint16_t w, h;
            tft.getTextBounds(name, screen_width / 2, displayHeight + animationY - animationDirection, &x1, &y1, &w, &h);
            // tft.fillRect(0, 0, screen_width, screen_height, TFT_BLACK); // Clear only the area where the text was
tft.fillScreen(TFT_BLACK);
            // Redisplay the text at the new position
            displayCenteredText(name, screen_width / 2, displayHeight + animationY, 4, false,true);
        }
        }
    if (animation == "invert"){
        // Invert the colors of the screen
        // use animationSpeed to control the speed of the animation
        // animationSpeed = 1 means BLACK every 100ms AND WHITE every 100ms
        // dont block with big delays
        if (millis() % 1000 < 500)
        {
            // tft.fillScreen(TFT_BLACK);
        tft.invertDisplay(true);
        }
        else
        {
            // tft.fillScreen(TFT_WHITE);
        tft.invertDisplay(false);
        }
        // tft.invertDisplay(true);
        // delay(1000);
        // tft.invertDisplay(false);
        // delay(1000);
    }
}

void setup()
{
    Serial.begin(115200);
    delay(2000);
    Serial.println("Initializing...");

    // Initialize hardware
    button_A.begin();
    button_B.begin();
    button_X.begin();
    button_Y.begin();
    button_MENU.begin();
    button_START.begin();

    spi->begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
    tft.init(TFT_WIDTH, TFT_HEIGHT);
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);

    Serial.println("Display and buttons initialized.");

    // Initialize NVS
    Serial.println("Attempting to initialize NVS...");
    if (preferences.begin("nametag-app", true))
    { // Open in read-only mode
        Serial.println("NVS initialized successfully");

        String storedName = preferences.getString("username", "");
        Serial.print("Stored name length: ");
        Serial.println(storedName.length());

        if (storedName.length() == 0)
        {
            Serial.println("No name stored. Starting setup.");
            state = "setup";
            name = "";
        }
        else
        {
            Serial.print("Stored name: ");
            Serial.println(storedName);
            state = "nametag";
            name = storedName;
        }

        preferences.end();
        Serial.println("NVS closed after reading");
    }
    else
    {
        Serial.println("Failed to initialize preferences");
        state = "setup";
        name = "";
    }

    drawGrid();
    highlightCursor(cursorX, cursorY);

    if (state == "nametag")
    {
        Serial.println("Displaying name tag...");
        nameTag(name);
    }

    Serial.println("Setup completed successfully");
}
void loop()
{
    static unsigned long lastDebugTime = 0;
    unsigned long currentTime = millis();

    if (state == "setup")
    {
        if (millis() % stability_delay == 0)
        {
            handleJoystick();
            checkButton(&button_A, "A");
            checkButton(&button_B, "B");
            checkButton(&button_X, "X");
            checkButton(&button_Y, "Y");
            checkButton(&button_MENU, "MENU");
            checkButton(&button_START, "START");
            }
        }
    else if (state == "nametag")
    {
        if (millis() % stability_delay == 0)
        {
            checkButton(&button_MENU, "MENU");
        }

    // Print debug info every 5 seconds
    if (currentTime - lastDebugTime > 5000)
    {
        Serial.println("Current state: " + state);
        Serial.println("Current name: " + name);
        Serial.println("Free heap: " + String(ESP.getFreeHeap()));
        lastDebugTime = currentTime;
    }

            // updateAnimation();
        }
    // delay(100); // Adjust delay for responsiveness
}
