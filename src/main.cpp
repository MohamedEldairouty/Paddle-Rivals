#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/freeglut.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>   // rand, srand, exit
#include <cmath>     // cosf, sinf, fabs
#include <ctime>     // time()

// ===================== GAME STATES =====================

enum GameState {
    STATE_MAIN_MENU,
    STATE_MODE_SELECT,
    STATE_DIFFICULTY_SELECT,
    STATE_NAME_INPUT_SINGLE,
    STATE_NAME_INPUT_MULTI_P1,
    STATE_NAME_INPUT_MULTI_P2,
    STATE_AVATAR_SELECT_SINGLE,
    STATE_AVATAR_SELECT_MULTI_P1,
    STATE_AVATAR_SELECT_MULTI_P2,
    STATE_HOW_TO_PLAY,
    STATE_SETTINGS,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER
};

GameState currentState = STATE_MAIN_MENU;

// ===================== MENU SELECTION =====================

int mainMenuIndex   = 0;   // 0..3 (Start, How to Play, Settings, Exit)
int modeMenuIndex   = 0;   // 0: Single, 1: Multiplayer
int difficultyIndex = 1;   // 0: Easy, 1: Medium, 2: Hard

// SETTINGS cursor: 0=GameTime, 1=MaxScore, 2=Theme, 3=Back
int settingsCursor  = 0;

// For avatar selection
int avatarCursor = 0;      // 0..3 for avatar index

// ===================== THEMES =====================

// 0=Neon, 1=Cosmic, 2=Retro  (Retro default)
int themeIndex       = 2;
const int themeCount = 3;
const char* themeNames[themeCount] = {
    "Neon Night",
    "Cosmic Field",
    "Retro Grid"
};

// Spinning cube angle (for 3D bonus)
float menuCubeAngle = 0.0f;

// ===================== WINDOW =====================

int winWidth  = 800;
int winHeight = 600;

// ===================== INPUT STATE (SMOOTH MOVEMENT) =====================

bool keyDown[256]      = { false }; // normal keys
bool specialDown[256]  = { false }; // special keys (arrows)

// ===================== GAME DATA STRUCTURES =====================

struct Paddle {
    float x, y;
    float width, height;
    float speed;
};

struct Ball {
    float x, y;
    float radius;
    float vx, vy;
};

Paddle p1, p2;
Ball   ball;

// Game parameters
int   scoreP1   = 0;
int   scoreP2   = 0;
float timeLeft  = 90.0f;   // set from settings
int   maxScore  = 5;       // set from settings

bool  isSinglePlayer = true;  // mode flag

// SETTINGS OPTIONS
const int gameTimeOptions[]   = { 60, 90, 120 };
const int gameTimeCount       = 3;
int       gameTimeIndex       = 1;  // default 90 sec

// maxScoreOptions: 0 means infinite
const int maxScoreOptions[]   = { 3, 5, 7, 0 };
const int maxScoreCount       = 4;
int       maxScoreIndex       = 1;  // default 5

// PLAYER NAMES
char player1Name[32] = "Player 1";
char player2Name[32] = "Player 2";

// Name input buffer
char nameBuffer[32] = "";
int  nameLength     = 0;

// ===== Ball speed system =====
float baseVx = 6.0f;    // base X speed
float baseVy = 4.0f;    // base Y speed
float speedFactor = 1.0f;   // grows in a rally
int   hitsInRally = 0;

float prevBallX = 0.0f, prevBallY = 0.0f;

// ===== Avatars =====

struct AvatarStyle {
    float r, g, b;
};

AvatarStyle avatarStyles[4] = {
    {0.2f, 0.8f, 1.0f}, // cyan
    {1.0f, 0.4f, 0.4f}, // red
    {0.3f, 1.0f, 0.5f}, // green
    {1.0f, 0.9f, 0.3f}  // yellow
};

int player1AvatarIndex = 0; // 0..3
int player2AvatarIndex = 1; // 0..3

// ===== Screen flash on score =====
int   flashFrames = 0;
float flashR = 1.0f, flashG = 1.0f, flashB = 1.0f;

// ===== Camera shake =====
int   shakeFrames = 0;
float shakeIntensity = 0.0f;

// ===================== SIMPLE TEXT RENDERING =====================

void drawBitmapText(const char* text, float x, float y, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (int i = 0; text[i] != '\0'; ++i) {
        glutBitmapCharacter(font, text[i]);
    }
}

void drawStrokeCentered(const char* text, float cx, float cy, float scale) {
    float width = 0.0f;
    for (int i = 0; text[i] != '\0'; ++i) {
        width += glutStrokeWidth(GLUT_STROKE_ROMAN, text[i]);
    }

    glPushMatrix();
        glTranslatef(cx - width * scale / 2.0f, cy, 0.0f);
        glScalef(scale, scale, 1.0f);
        for (int i = 0; text[i] != '\0'; ++i) {
            glutStrokeCharacter(GLUT_STROKE_ROMAN, text[i]);
        }
    glPopMatrix();
}

// ===================== SCENE HELPERS =====================

void setup2D() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, winWidth, 0, winHeight);  // origin bottom-left
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void drawRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
        glVertex2f(x,     y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x,     y + h);
    glEnd();
}

void drawCircle(float cx, float cy, float r) {
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i < 50; i++) {
        float a = i * 2.0f * 3.14159f / 50.0f;
        glVertex2f(cx + cosf(a)*r, cy + sinf(a)*r);
    }
    glEnd();
}

void drawHexagon(float cx, float cy, float r) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < 6; ++i) {
        float a = i * 2.0f * 3.14159f / 6.0f;
        glVertex2f(cx + cosf(a)*r, cy + sinf(a)*r);
    }
    glEnd();
}

void drawTriangle(float cx, float cy, float r) {
    glBegin(GL_TRIANGLES);
        glVertex2f(cx,     cy + r);
        glVertex2f(cx - r, cy - r);
        glVertex2f(cx + r, cy - r);
    glEnd();
}

// =========== BACKGROUNDS (THEMED) ===========

void drawMenuBackground() {
    // Darker gradients for all themes (better contrast)
    for (int i = 0; i < winHeight; i += 25) {
        float t = (float)i / (float)winHeight;

        float r,g,b;

        if (themeIndex == 0) { // Neon Night
            r = 0.01f + 0.05f * t;
            g = 0.02f + 0.10f * t;
            b = 0.05f + 0.30f * (1.0f - t);
        } else if (themeIndex == 1) { // Cosmic Field
            r = 0.01f + 0.04f * t;
            g = 0.01f + 0.06f * (1.0f - t);
            b = 0.08f + 0.25f * t;
        } else { // Retro Grid
            r = 0.06f + 0.20f * t;
            g = 0.01f + 0.05f * (1.0f - t);
            b = 0.07f + 0.18f * (1.0f - t);
        }

        glColor3f(r, g, b);
        drawRect(0, (float)i, (float)winWidth, 25.0f);
    }

    // Top highlight bar
    if (themeIndex == 0)      glColor3f(0.2f, 0.9f, 1.0f);
    else if (themeIndex == 1) glColor3f(0.7f, 0.8f, 1.0f);
    else                      glColor3f(1.0f, 0.7f, 0.3f);

    drawRect(0, winHeight - 8, winWidth, 8);
}

void drawGameBackground() {
    if (themeIndex == 2) {
        // Retro Grid: dark purple + neon grid
        glColor3f(0.03f, 0.0f, 0.05f);
        drawRect(0, 0, winWidth, winHeight);

        glColor3f(0.5f, 0.0f, 0.7f);
        for (int y = 0; y < winHeight; y += 30) {
            drawRect(0, (float)y, (float)winWidth, 1.5f);
        }
        for (int x = 0; x < winWidth; x += 40) {
            drawRect((float)x, 0.0f, 1.5f, (float)winHeight);
        }
    } else {
        // Gradient field (Neon / Cosmic)
        for (int i = 0; i < winHeight; i += 20) {
            float t = (float)i / (float)winHeight;
            float r,g,b;
            if (themeIndex == 0) { // Neon
                r = 0.01f + 0.08f * t;
                g = 0.03f + 0.18f * t;
                b = 0.06f + 0.22f * (1.0f - t);
            } else { // Cosmic
                r = 0.01f + 0.05f * t;
                g = 0.01f + 0.05f * (1.0f - t);
                b = 0.10f + 0.30f * t;
            }
            glColor3f(r, g, b);
            drawRect(0, (float)i, (float)winWidth, 20.0f);
        }
    }

    // Center dashed line
    glColor3f(0.9f, 0.9f, 0.9f);
    float cx = winWidth / 2.0f - 2.0f;
    float dashH = 16.0f;
    float gapH  = 10.0f;
    for (float y = 0; y < winHeight; y += dashH + gapH) {
        drawRect(cx, y, 4.0f, dashH);
    }
}

// ===================== 3D CUBES (BONUS) =====================

void drawSpinningCube(float tx, float ty, float tz, float size, float angle) {
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(45.0, (double)winWidth / (double)winHeight, 1.0, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    gluLookAt(0.0, 0.0, 10.0,
              0.0, 0.0, 0.0,
              0.0, 1.0, 0.0);

    glTranslatef(tx, ty, tz);
    glRotatef(angle, 1.0f, 1.0f, 0.0f);

    // Color depends on theme
    if (themeIndex == 0)      glColor3f(0.1f, 0.9f, 1.0f);
    else if (themeIndex == 1) glColor3f(0.7f, 0.9f, 1.0f);
    else                      glColor3f(1.0f, 0.6f, 0.2f);

    float s = size;
    glBegin(GL_QUADS);
        // Front
        glVertex3f(-s, -s,  s);
        glVertex3f( s, -s,  s);
        glVertex3f( s,  s,  s);
        glVertex3f(-s,  s,  s);
        // Back
        glVertex3f(-s, -s, -s);
        glVertex3f(-s,  s, -s);
        glVertex3f( s,  s, -s);
        glVertex3f( s, -s, -s);
        // Left
        glVertex3f(-s, -s, -s);
        glVertex3f(-s, -s,  s);
        glVertex3f(-s,  s,  s);
        glVertex3f(-s,  s, -s);
        // Right
        glVertex3f(s, -s, -s);
        glVertex3f(s,  s, -s);
        glVertex3f(s,  s,  s);
        glVertex3f(s, -s,  s);
        // Top
        glVertex3f(-s,  s, -s);
        glVertex3f(-s,  s,  s);
        glVertex3f( s,  s,  s);
        glVertex3f( s,  s, -s);
        // Bottom
        glVertex3f(-s, -s, -s);
        glVertex3f( s, -s, -s);
        glVertex3f( s, -s,  s);
        glVertex3f(-s, -s,  s);
    glEnd();

    glPopMatrix(); // modelview
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_DEPTH_TEST);
}

void drawMenuCube3D() {
    // cube behind title
    drawSpinningCube(0.0f, 1.0f, 0.0f, 2.0f, menuCubeAngle);
}

void drawGameCube3D() {
    // subtle cube in game center behind field
    drawSpinningCube(0.0f, -0.5f, -2.0f, 1.2f, menuCubeAngle * 1.5f);
}

// ===================== GAME INIT =====================

void resetBall() {
    hitsInRally = 0;
    speedFactor = 1.0f;

    ball.x = winWidth / 2.0f;
    ball.y = winHeight / 2.0f;

    float vx = baseVx;
    float vy = baseVy;

    ball.vx = (rand() % 2 == 0 ? vx : -vx);
    ball.vy = (rand() % 2 == 0 ? vy : -vy);

    prevBallX = ball.x;
    prevBallY = ball.y;
}

void initRoundObjects(bool resetScores) {
    // Paddles
    p1.x = 80.0f;
    p1.y = winHeight / 2.0f;
    p1.width  = 16.0f;
    p1.height = 100.0f;
    p1.speed  = 8.0f;

    p2.x = winWidth - 80.0f;
    p2.y = winHeight / 2.0f;
    p2.width  = 16.0f;
    p2.height = 100.0f;
    p2.speed  = 8.0f;

    if (resetScores) {
        scoreP1 = 0;
        scoreP2 = 0;
    }

    ball.radius = 12.0f;
    resetBall();
}

void applySettingsToGame() {
    timeLeft = (float)gameTimeOptions[gameTimeIndex];
    maxScore = maxScoreOptions[maxScoreIndex]; // 0 means infinite
}

void startNewMatch() {
    applySettingsToGame();
    initRoundObjects(true);
}

// ===================== Music =====================

void stopBackgroundMusic(); // forward decl

void startBackgroundMusic() {
    BOOL ok = PlaySoundA("D:\\Prog\\C++\\Graphics\\Paddle Rivals\\bg_music.wav",
                         NULL,
                         SND_ASYNC | SND_LOOP | SND_FILENAME);
    if (!ok) {
        MessageBoxA(NULL, "Failed to load bg_music.wav", "ERROR", MB_OK);
    }
}

void stopBackgroundMusic() {
    PlaySoundA(NULL, NULL, 0); // stop any playing sound
}

// ===================== MAIN MENU =====================

void drawMainMenu() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setup2D();

    drawMenuBackground();
    drawMenuCube3D();

    // Title color per theme (high contrast)
    if (themeIndex == 0)      glColor3f(1.0f, 0.3f, 0.9f);   // neon magenta
    else if (themeIndex == 1) glColor3f(0.4f, 1.0f, 0.6f);   // lime
    else                      glColor3f(0.3f, 1.0f, 1.0f);   // cyan

    drawStrokeCentered("PADDLE RIVALS", winWidth/2.0f, winHeight - 130.0f, 0.20f);

    const char* options[] = {
        "Start New Game",
        "How to Play",
        "Settings",
        "Exit"
    };
    const int optionCount = 4;

    float startY  = winHeight / 2.0f + 60.0f;
    float textX   = winWidth / 2.0f - 80.0f;

    for (int i = 0; i < optionCount; ++i) {
        float y = startY - i * 40.0f;

        if (i == mainMenuIndex) {
            // --- Highlight background bar ---
            if (themeIndex == 0) {          // Neon Night → bright cyan bar
                glColor3f(0.0f, 1.0f, 0.7f);
            } else if (themeIndex == 1) {   // Cosmic Field → blue bar
                glColor3f(0.4f, 0.7f, 1.0f);
            } else {                        // Retro Grid → orange bar
                glColor3f(1.0f, 0.6f, 0.2f);
            }

            // Background rectangle behind text
            drawRect(textX - 40.0f, y - 8.0f, 260.0f, 24.0f);

            // Optional: ">" arrow indicator
            glColor3f(0.0f, 0.0f, 0.0f);
            drawBitmapText(">", textX - 30.0f, y);

            // Selected text in dark color for contrast
            glColor3f(0.0f, 0.0f, 0.0f);
            drawBitmapText(options[i], textX, y);
        } else {
            // Non-selected options: light text, no bar
            glColor3f(0.9f, 0.9f, 0.95f);
            drawBitmapText(options[i], textX, y);
        }
    }

    glutSwapBuffers();
}

void handleEnterOnMainMenu() {
    switch (mainMenuIndex) {
        case 0: currentState = STATE_MODE_SELECT; break;
        case 1: currentState = STATE_HOW_TO_PLAY; break;
        case 2: currentState = STATE_SETTINGS;    break;
        case 3:
            stopBackgroundMusic();
            std::exit(0);
            break;
    }
}

// ===================== MODE SELECT =====================

void drawModeSelectMenu() {
    glClear(GL_COLOR_BUFFER_BIT);
    setup2D();

    drawMenuBackground();

    glColor3f(0.9f, 0.9f, 1.0f);
    drawBitmapText("SELECT MODE", winWidth/2 - 80, winHeight - 100);

    const char* modes[] = {
        "Single Player",
        "Multiplayer"
    };

    for (int i = 0; i < 2; ++i) {
        if (i == modeMenuIndex)
            glColor3f(0.2f, 0.8f, 1.0f);
        else
            glColor3f(0.7f, 0.7f, 0.8f);

        drawBitmapText(modes[i], winWidth/2 - 80, winHeight/2 + 20 - i * 40);
    }

    glColor3f(0.6f, 0.6f, 0.7f);
    drawBitmapText("Press ESC to go back", 20, 20);

    glutSwapBuffers();
}

void handleEnterOnModeMenu() {
    isSinglePlayer = (modeMenuIndex == 0);
    if (isSinglePlayer) {
        currentState = STATE_DIFFICULTY_SELECT;
    } else {
        nameBuffer[0] = '\0';
        nameLength = 0;
        currentState = STATE_NAME_INPUT_MULTI_P1;
    }
}

// ===================== DIFFICULTY SELECT =====================

void drawDifficultySelect() {
    glClear(GL_COLOR_BUFFER_BIT);
    setup2D();

    drawMenuBackground();

    glColor3f(0.9f, 0.9f, 1.0f);
    drawBitmapText("SELECT DIFFICULTY", winWidth/2 - 110, winHeight - 90);

    const char* labels[] = { "Easy", "Medium", "Hard" };
    float startY = winHeight / 2.0f + 40.0f;

    for (int i = 0; i < 3; ++i) {
        if (i == difficultyIndex)
            glColor3f(0.2f, 0.8f, 1.0f);
        else
            glColor3f(0.7f, 0.7f, 0.8f);

        drawBitmapText(labels[i], winWidth/2 - 40, startY - i * 40);
    }

    glColor3f(0.6f, 0.6f, 0.7f);
    drawBitmapText("Use Up/Down to choose, Enter to continue, ESC to go back", 60, 40);

    glutSwapBuffers();
}

// ===================== NAME INPUT SCREENS =====================

void drawNameInputScreen(const char* title, const char* label) {
    glClear(GL_COLOR_BUFFER_BIT);
    setup2D();

    drawMenuBackground();

    glColor3f(0.9f, 0.9f, 1.0f);
    drawBitmapText(title, winWidth/2 - 100, winHeight - 80);

    glColor3f(0.8f, 0.8f, 0.9f);
    drawBitmapText(label, winWidth/2 - 150, winHeight/2 + 20);

    glColor3f(0.2f, 0.2f, 0.3f);
    drawRect(winWidth/2 - 150, winHeight/2 - 10, 300, 30);

    glColor3f(1.0f, 1.0f, 1.0f);
    drawBitmapText(nameBuffer, winWidth/2 - 140, winHeight/2);

    glColor3f(0.6f, 0.6f, 0.7f);
    drawBitmapText("Type name, Enter to confirm, ESC to cancel/back", winWidth/2 - 170, 40);

    glutSwapBuffers();
}

void finishSingleNameInput() {
    if (nameLength == 0) std::strcpy(player1Name, "Player 1");
    else {
        std::strncpy(player1Name, nameBuffer, sizeof(player1Name)-1);
        player1Name[sizeof(player1Name)-1] = '\0';
    }
    avatarCursor = player1AvatarIndex;
    currentState = STATE_AVATAR_SELECT_SINGLE;
}

void finishMultiNameP1() {
    if (nameLength == 0) std::strcpy(player1Name, "Player 1");
    else {
        std::strncpy(player1Name, nameBuffer, sizeof(player1Name)-1);
        player1Name[sizeof(player1Name)-1] = '\0';
    }
    nameBuffer[0] = '\0';
    nameLength = 0;
    avatarCursor = player1AvatarIndex;
    currentState = STATE_AVATAR_SELECT_MULTI_P1;
}

void finishMultiNameP2() {
    if (nameLength == 0) std::strcpy(player2Name, "Player 2");
    else {
        std::strncpy(player2Name, nameBuffer, sizeof(player2Name)-1);
        player2Name[sizeof(player2Name)-1] = '\0';
    }
    avatarCursor = player2AvatarIndex;
    currentState = STATE_AVATAR_SELECT_MULTI_P2;
}

// ===================== AVATAR SELECT SCREENS =====================

void drawAvatarPreview(float cx, float cy, int index, bool highlight) {
    AvatarStyle style = avatarStyles[index];

    if (highlight) {
        glColor3f(1.0f, 1.0f, 1.0f);
        drawCircle(cx, cy, 40.0f);
    }

    glColor3f(style.r, style.g, style.b);

    switch (index) {
        case 0: // Circle logo
            drawCircle(cx, cy, 28.0f);
            glColor3f(0.0f, 0.0f, 0.0f);
            drawCircle(cx, cy, 12.0f);
            break;
        case 1: // Shield
            glBegin(GL_POLYGON);
                glVertex2f(cx - 22.0f, cy + 24.0f);
                glVertex2f(cx + 22.0f, cy + 24.0f);
                glVertex2f(cx + 18.0f, cy - 10.0f);
                glVertex2f(cx,          cy - 28.0f);
                glVertex2f(cx - 18.0f, cy - 10.0f);
            glEnd();
            break;
        case 2: // Star
            glBegin(GL_TRIANGLE_FAN);
            for (int i = 0; i < 10; ++i) {
                float a = i * 2.0f * 3.14159f / 10.0f;
                float r = (i % 2 == 0) ? 30.0f : 14.0f;
                glVertex2f(cx + cosf(a)*r, cy + sinf(a)*r);
            }
            glEnd();
            break;
        case 3: // Hexagon
            drawHexagon(cx, cy, 28.0f);
            break;
    }
}

void drawAvatarSelectScreen(const char* title, const char* subtitle) {
    glClear(GL_COLOR_BUFFER_BIT);
    setup2D();

    drawMenuBackground();

    glColor3f(0.9f, 0.9f, 1.0f);
    drawBitmapText(title,    winWidth/2 - 100, winHeight - 80);
    drawBitmapText(subtitle, winWidth/2 - 150, winHeight - 120);

    float centerY = winHeight / 2.0f;
    float spacing = 140.0f;
    float startX  = winWidth/2.0f - 1.5f * spacing;

    for (int i = 0; i < 4; ++i) {
        bool highlight = (i == avatarCursor);
        drawAvatarPreview(startX + i*spacing, centerY, i, highlight);
    }

    glColor3f(0.8f, 0.8f, 0.9f);
    drawBitmapText("Use LEFT/RIGHT to choose, ENTER to confirm, ESC to go back", 80, 60);

    glutSwapBuffers();
}

void finishAvatarSingle() {
    player1AvatarIndex = avatarCursor;

    if      (difficultyIndex == 0) std::strcpy(player2Name, "AI (Easy)");
    else if (difficultyIndex == 1) std::strcpy(player2Name, "AI (Medium)");
    else                           std::strcpy(player2Name, "AI (Hard)");

    isSinglePlayer = true;
    startNewMatch();
    startBackgroundMusic();        // 🔊 start music when match actually starts
    currentState = STATE_PLAYING;
}

void finishAvatarMultiP1() {
    player1AvatarIndex = avatarCursor;
    nameBuffer[0] = '\0';
    nameLength = 0;
    currentState = STATE_NAME_INPUT_MULTI_P2;
}

void finishAvatarMultiP2() {
    player2AvatarIndex = avatarCursor;
    isSinglePlayer = false;
    startNewMatch();
    startBackgroundMusic();        // 🔊 start music for multiplayer match
    currentState = STATE_PLAYING;
}

// ===================== HOW TO PLAY =====================

void drawHowToPlay() {
    glClear(GL_COLOR_BUFFER_BIT);
    setup2D();

    drawMenuBackground();

    glColor3f(0.9f, 0.9f, 1.0f);
    drawBitmapText("HOW TO PLAY", winWidth/2 - 70, winHeight - 80);

    glColor3f(0.8f, 0.8f, 0.9f);
    int y = winHeight - 140;
    drawBitmapText("- Paddle Rivals is a 3D Pong-style rivalry game.",                60, y); y -= 30;
    drawBitmapText("- Player 1: W/S for up/down, A/D for left/right.",               60, y); y -= 30;
    drawBitmapText("- In Single Player: Arrow keys also move Player 1.",             60, y); y -= 30;
    drawBitmapText("- Multiplayer: Arrow keys move Player 2.",                       60, y); y -= 30;
    drawBitmapText("- Avatars & colors are chosen in the avatar screen.",            60, y); y -= 30;
    drawBitmapText("- Score by sending the ball past the opponent.",                 60, y); y -= 30;
    drawBitmapText("- Game Time & Max Score can be set from Settings.",              60, y); y -= 30;
    drawBitmapText("- Difficulty affects only the AI in Single Player.",             60, y); y -= 30;

    glColor3f(0.6f, 0.6f, 0.7f);
    drawBitmapText("Press ESC to return to Main Menu", 60, 40);

    glutSwapBuffers();
}

// ===================== SETTINGS =====================

void drawSettings() {
    glClear(GL_COLOR_BUFFER_BIT);
    setup2D();

    drawMenuBackground();

    glColor3f(0.9f, 0.9f, 1.0f);
    drawBitmapText("SETTINGS", winWidth/2 - 50, winHeight - 80);

    char line[64];

    int y = winHeight - 150;

    // Game Time
    if (settingsCursor == 0) glColor3f(0.2f, 0.8f, 1.0f);
    else                     glColor3f(0.9f, 0.9f, 0.95f);
    std::sprintf(line, "Game Time: %d sec", gameTimeOptions[gameTimeIndex]);
    drawBitmapText(line, 80, y);

    // Max Score
    y -= 40;
    if (settingsCursor == 1) glColor3f(0.2f, 0.8f, 1.0f);
    else                     glColor3f(0.9f, 0.9f, 0.95f);
    int msVal = maxScoreOptions[maxScoreIndex];
    if (msVal == 0) std::sprintf(line, "Max Score: Infinite");
    else            std::sprintf(line, "Max Score: %d", msVal);
    drawBitmapText(line, 80, y);

    // Theme
    y -= 40;
    if (settingsCursor == 2) glColor3f(0.2f, 0.8f, 1.0f);
    else                     glColor3f(0.9f, 0.9f, 0.95f);
    std::sprintf(line, "Theme: %s", themeNames[themeIndex]);
    drawBitmapText(line, 80, y);

    // Back
    y -= 40;
    if (settingsCursor == 3) glColor3f(0.2f, 0.8f, 1.0f);
    else                     glColor3f(0.9f, 0.9f, 0.95f);
    drawBitmapText("Back to Main Menu", 80, y);

    glColor3f(0.6f, 0.6f, 0.7f);
    drawBitmapText("Use Up/Down to select, Left/Right to change, Enter/ESC to go back.", 40, 40);

    glutSwapBuffers();
}

// ===================== GAME RENDERING =====================

void drawAvatarHUD(float x, float y, int avatarIndex) {
    AvatarStyle style = avatarStyles[avatarIndex];

    glColor3f(0.0f, 0.0f, 0.0f);
    drawCircle(x + 4, y - 4, 22.0f);

    glColor3f(style.r, style.g, style.b);
    switch (avatarIndex) {
        case 0: drawCircle(x, y, 20.0f); break;
        case 1:
            glBegin(GL_POLYGON);
                glVertex2f(x - 18.0f, y + 20.0f);
                glVertex2f(x + 18.0f, y + 20.0f);
                glVertex2f(x + 14.0f, y - 5.0f);
                glVertex2f(x,          y - 20.0f);
                glVertex2f(x - 14.0f, y - 5.0f);
            glEnd();
            break;
        case 2: drawTriangle(x, y, 22.0f); break;
        case 3: drawHexagon(x, y, 20.0f);  break;
    }
}

void drawGame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 3D object behind the field
    drawGameCube3D();

    setup2D();

    // --- Camera shake offsets ---
    float ox = 0.0f, oy = 0.0f;
    if (shakeFrames > 0) {
        ox = ((rand() % 100) / 100.0f - 0.5f) * shakeIntensity;
        oy = ((rand() % 100) / 100.0f - 0.5f) * shakeIntensity;
    }

    glPushMatrix();
    glTranslatef(ox, oy, 0.0f);

    drawGameBackground();

    // Shadows for 3D-ish feel
    glColor3f(0.0f, 0.0f, 0.0f);
    drawRect(p1.x - p1.width/2 + 6, p1.y - p1.height/2 - 6, p1.width, p1.height);
    drawRect(p2.x - p2.width/2 + 6, p2.y - p2.height/2 - 6, p2.width, p2.height);
    drawCircle(ball.x + 5, ball.y - 5, ball.radius);

    AvatarStyle s1 = avatarStyles[player1AvatarIndex];
    AvatarStyle s2 = avatarStyles[player2AvatarIndex];

    glColor3f(s1.r, s1.g, s1.b);
    drawRect(p1.x - p1.width/2, p1.y - p1.height/2, p1.width, p1.height);

    glColor3f(s2.r, s2.g, s2.b);
    drawRect(p2.x - p2.width/2, p2.y - p2.height/2, p2.width, p2.height);

    // Ball glow (theme-based)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (themeIndex == 0)      glColor4f(0.2f, 1.0f, 1.0f, 0.4f);
    else if (themeIndex == 1) glColor4f(0.7f, 0.7f, 1.0f, 0.4f);
    else                      glColor4f(1.0f, 0.5f, 0.2f, 0.4f);
    drawCircle(ball.x, ball.y, ball.radius + 8.0f);
    glDisable(GL_BLEND);

    // Ball core
    glColor3f(1.0f, 1.0f, 1.0f);
    drawCircle(ball.x, ball.y, ball.radius);

    // HUD
    drawAvatarHUD(60.0f, winHeight - 45.0f, player1AvatarIndex);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawBitmapText(player1Name, 100.0f, winHeight - 52.0f);

    drawAvatarHUD(winWidth - 60.0f, winHeight - 45.0f, player2AvatarIndex);
    drawBitmapText(player2Name, winWidth - 200.0f, winHeight - 52.0f);

    char scoreText[64];
    std::sprintf(scoreText, "%d  :  %d", scoreP1, scoreP2);
    drawBitmapText(scoreText, winWidth/2 - 20, winHeight - 52.0f);

    char timeText[32];
    std::sprintf(timeText, "Time: %d", (int)timeLeft);
    drawBitmapText(timeText, winWidth/2 - 40, winHeight - 80.0f);

    // Screen flash overlay (also shaken)
    if (flashFrames > 0) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(flashR, flashG, flashB, 0.25f);
        drawRect(0, 0, winWidth, winHeight);
        glDisable(GL_BLEND);
    }

    glPopMatrix();

    glutSwapBuffers();
}

// ===================== PAUSED & GAME OVER =====================

void drawPaused() {
    glClear(GL_COLOR_BUFFER_BIT);
    setup2D();

    drawGameBackground();

    glColor3f(0.0f, 0.0f, 0.0f);
    drawRect(winWidth/2 - 160, winHeight/2 - 80, 320, 160);

    glColor3f(0.9f, 0.9f, 1.0f);
    drawBitmapText("PAUSED", winWidth/2 - 40, winHeight/2 + 40);

    glColor3f(0.7f, 0.7f, 0.8f);
    drawBitmapText("Press ESC to Resume",        winWidth/2 - 80, winHeight/2);
    drawBitmapText("Press M to go to Main Menu", winWidth/2 - 110, winHeight/2 - 30);

    glutSwapBuffers();
}

void drawGameOver() {
    glClear(GL_COLOR_BUFFER_BIT);
    setup2D();

    drawGameBackground();

    glColor3f(0.0f, 0.0f, 0.0f);
    drawRect(winWidth/2 - 200, winHeight/2 - 90, 400, 180);

    glColor3f(1.0f, 0.8f, 0.8f);
    drawBitmapText("GAME OVER", winWidth/2 - 60, winHeight/2 + 40);

    char result[64];
    if (scoreP1 > scoreP2)
        std::sprintf(result, "Winner: %s (%d : %d)", player1Name, scoreP1, scoreP2);
    else if (scoreP2 > scoreP1)
        std::sprintf(result, "Winner: %s (%d : %d)", player2Name, scoreP2, scoreP1);
    else
        std::sprintf(result, "Draw! (%d : %d)", scoreP1, scoreP2);

    drawBitmapText(result, winWidth/2 - 140, winHeight/2 - 10);
    drawBitmapText("Press M for Main Menu",      winWidth/2 - 90,  winHeight/2 - 40);

    glutSwapBuffers();
}

// ===================== DISPLAY CALLBACK =====================

void displayCallback() {
    switch (currentState) {
        case STATE_MAIN_MENU:              drawMainMenu();                        break;
        case STATE_MODE_SELECT:            drawModeSelectMenu();                  break;
        case STATE_DIFFICULTY_SELECT:      drawDifficultySelect();                break;
        case STATE_NAME_INPUT_SINGLE:      drawNameInputScreen("SINGLE PLAYER", "Enter your name:"); break;
        case STATE_NAME_INPUT_MULTI_P1:    drawNameInputScreen("MULTIPLAYER",     "Player 1 name:"); break;
        case STATE_NAME_INPUT_MULTI_P2:    drawNameInputScreen("MULTIPLAYER",     "Player 2 name:"); break;
        case STATE_AVATAR_SELECT_SINGLE:   drawAvatarSelectScreen("AVATAR SELECT", "Choose your logo:"); break;
        case STATE_AVATAR_SELECT_MULTI_P1: drawAvatarSelectScreen("AVATAR SELECT", "Player 1 - choose your logo:"); break;
        case STATE_AVATAR_SELECT_MULTI_P2: drawAvatarSelectScreen("AVATAR SELECT", "Player 2 - choose your logo:"); break;
        case STATE_HOW_TO_PLAY:            drawHowToPlay();                       break;
        case STATE_SETTINGS:               drawSettings();                        break;
        case STATE_PLAYING:                drawGame();                            break;
        case STATE_PAUSED:                 drawPaused();                          break;
        case STATE_GAME_OVER:              drawGameOver();                        break;
    }
}

// ===================== RESHAPE =====================

void reshapeCallback(int w, int h) {
    winWidth  = (w > 0) ? w : 1;
    winHeight = (h > 0) ? h : 1;
    glViewport(0, 0, winWidth, winHeight);
}

// ===================== NAME INPUT KEYBOARD =====================

void handleNameInputKey(unsigned char key) {
    if (key == 27) {
        if (currentState == STATE_NAME_INPUT_SINGLE)
            currentState = STATE_DIFFICULTY_SELECT;
        else
            currentState = STATE_MODE_SELECT;
    } else if (key == 13) {
        if (currentState == STATE_NAME_INPUT_SINGLE)
            finishSingleNameInput();
        else if (currentState == STATE_NAME_INPUT_MULTI_P1)
            finishMultiNameP1();
        else
            finishMultiNameP2();
    } else if (key == 8) {
        if (nameLength > 0) {
            nameLength--;
            nameBuffer[nameLength] = '\0';
        }
    } else if (key >= 32 && key <= 126) {
        if (nameLength < (int)sizeof(nameBuffer) - 1) {
            nameBuffer[nameLength++] = key;
            nameBuffer[nameLength]   = '\0';
        }
    }
}

// ===================== KEYBOARD INPUT =====================

void keyboardCallback(unsigned char key, int x, int y) {
    keyDown[(unsigned char)key] = true;

    switch (currentState) {
        case STATE_MAIN_MENU:
            if (key == 13) handleEnterOnMainMenu();
            break;

        case STATE_MODE_SELECT:
            if (key == 13) handleEnterOnModeMenu();
            else if (key == 27) currentState = STATE_MAIN_MENU;
            break;

        case STATE_DIFFICULTY_SELECT:
            if (key == 13) {
                nameBuffer[0] = '\0';
                nameLength = 0;
                currentState = STATE_NAME_INPUT_SINGLE;
            } else if (key == 27) {
                currentState = STATE_MODE_SELECT;
            }
            break;

        case STATE_NAME_INPUT_SINGLE:
        case STATE_NAME_INPUT_MULTI_P1:
        case STATE_NAME_INPUT_MULTI_P2:
            handleNameInputKey(key);
            break;

        case STATE_AVATAR_SELECT_SINGLE:
            if (key == 13) finishAvatarSingle();
            else if (key == 27) currentState = STATE_NAME_INPUT_SINGLE;
            break;

        case STATE_AVATAR_SELECT_MULTI_P1:
            if (key == 13) finishAvatarMultiP1();
            else if (key == 27) currentState = STATE_NAME_INPUT_MULTI_P1;
            break;

        case STATE_AVATAR_SELECT_MULTI_P2:
            if (key == 13) finishAvatarMultiP2();
            else if (key == 27) currentState = STATE_NAME_INPUT_MULTI_P2;
            break;

        case STATE_HOW_TO_PLAY:
            if (key == 27) currentState = STATE_MAIN_MENU;
            break;

        case STATE_SETTINGS:
            if (key == 27) currentState = STATE_MAIN_MENU;
            else if (key == 13 && settingsCursor == 3) currentState = STATE_MAIN_MENU;
            break;

        case STATE_PLAYING:
            if (key == 27) currentState = STATE_PAUSED;
            break;

        case STATE_PAUSED:
            if (key == 27) {
                currentState = STATE_PLAYING;
            } else if (key == 'm' || key == 'M') {
                stopBackgroundMusic();            // 🔇 back to menu
                currentState = STATE_MAIN_MENU;
            }
            break;

        case STATE_GAME_OVER:
            if (key == 'm' || key == 'M') {
                stopBackgroundMusic();            // 🔇 back to menu
                currentState = STATE_MAIN_MENU;
            }
            break;
    }

    glutPostRedisplay();
}

void keyboardUpCallback(unsigned char key, int x, int y) {
    keyDown[(unsigned char)key] = false;
}

// Special keys (arrows)
void specialCallback(int key, int x, int y) {
    specialDown[key] = true;

    switch (currentState) {
        case STATE_MAIN_MENU:
            if (key == GLUT_KEY_UP) {
                mainMenuIndex--;
                if (mainMenuIndex < 0) mainMenuIndex = 3;
            } else if (key == GLUT_KEY_DOWN) {
                mainMenuIndex++;
                if (mainMenuIndex > 3) mainMenuIndex = 0;
            }
            break;

        case STATE_MODE_SELECT:
            if (key == GLUT_KEY_UP || key == GLUT_KEY_DOWN) {
                modeMenuIndex = 1 - modeMenuIndex;
            }
            break;

        case STATE_DIFFICULTY_SELECT:
            if (key == GLUT_KEY_UP) {
                difficultyIndex--;
                if (difficultyIndex < 0) difficultyIndex = 2;
            } else if (key == GLUT_KEY_DOWN) {
                difficultyIndex++;
                if (difficultyIndex > 2) difficultyIndex = 0;
            }
            break;

        case STATE_SETTINGS:
            if (key == GLUT_KEY_UP) {
                settingsCursor--;
                if (settingsCursor < 0) settingsCursor = 3;
            } else if (key == GLUT_KEY_DOWN) {
                settingsCursor++;
                if (settingsCursor > 3) settingsCursor = 0;
            } else if (key == GLUT_KEY_LEFT) {
                if (settingsCursor == 0) {
                    gameTimeIndex--;
                    if (gameTimeIndex < 0) gameTimeIndex = gameTimeCount - 1;
                } else if (settingsCursor == 1) {
                    maxScoreIndex--;
                    if (maxScoreIndex < 0) maxScoreIndex = maxScoreCount - 1;
                } else if (settingsCursor == 2) {
                    themeIndex--;
                    if (themeIndex < 0) themeIndex = themeCount - 1;
                }
            } else if (key == GLUT_KEY_RIGHT) {
                if (settingsCursor == 0) {
                    gameTimeIndex++;
                    if (gameTimeIndex >= gameTimeCount) gameTimeIndex = 0;
                } else if (settingsCursor == 1) {
                    maxScoreIndex++;
                    if (maxScoreIndex >= maxScoreCount) maxScoreIndex = 0;
                } else if (settingsCursor == 2) {
                    themeIndex++;
                    if (themeIndex >= themeCount) themeIndex = 0;
                }
            }
            break;

        case STATE_AVATAR_SELECT_SINGLE:
        case STATE_AVATAR_SELECT_MULTI_P1:
        case STATE_AVATAR_SELECT_MULTI_P2:
            if (key == GLUT_KEY_LEFT) {
                avatarCursor--;
                if (avatarCursor < 0) avatarCursor = 3;
            } else if (key == GLUT_KEY_RIGHT) {
                avatarCursor++;
                if (avatarCursor > 3) avatarCursor = 0;
            }
            break;

        default:
            break;
    }

    glutPostRedisplay();
}

void specialUpCallback(int key, int x, int y) {
    specialDown[key] = false;
}

// ===================== TIMER / GAME LOOP =====================

void timerCallback(int value) {
    // 3D cube spin
    menuCubeAngle += 0.7f;
    if (menuCubeAngle > 360.0f) menuCubeAngle -= 360.0f;

    if (currentState == STATE_PLAYING) {

        // --- PLAYER 1 movement (no double-speed bug) ---
        float moveX1 = 0.0f, moveY1 = 0.0f;

        if (keyDown['w'] || keyDown['W']) moveY1 += 1.0f;
        if (keyDown['s'] || keyDown['S']) moveY1 -= 1.0f;
        if (keyDown['a'] || keyDown['A']) moveX1 -= 1.0f;
        if (keyDown['d'] || keyDown['D']) moveX1 += 1.0f;

        if (isSinglePlayer) {
            if (specialDown[GLUT_KEY_UP])    moveY1 += 1.0f;
            if (specialDown[GLUT_KEY_DOWN])  moveY1 -= 1.0f;
            if (specialDown[GLUT_KEY_LEFT])  moveX1 -= 1.0f;
            if (specialDown[GLUT_KEY_RIGHT]) moveX1 += 1.0f;
        }

        if (moveX1 > 1.0f)  moveX1 = 1.0f;
        if (moveX1 < -1.0f) moveX1 = -1.0f;
        if (moveY1 > 1.0f)  moveY1 = 1.0f;
        if (moveY1 < -1.0f) moveY1 = -1.0f;

        p1.x += moveX1 * p1.speed;
        p1.y += moveY1 * p1.speed;

        // --- PLAYER 2 movement ---
        if (isSinglePlayer) {
            float aiBaseSpeed;
            if      (difficultyIndex == 0) aiBaseSpeed = 5.0f;
            else if (difficultyIndex == 1) aiBaseSpeed = 8.0f;
            else                           aiBaseSpeed = 11.0f;

            float dy = ball.y - p2.y;
            float ay = aiBaseSpeed;
            if (dy > ay)        p2.y += ay;
            else if (dy < -ay)  p2.y -= ay;
            else                p2.y = ball.y;

            float p2MinX = winWidth / 2.0f + 60.0f;
            float p2MaxX = winWidth - 40.0f;
            float targetX;

            if (ball.x > winWidth / 2.0f) {
                targetX = ball.x;
                if (targetX < p2MinX) targetX = p2MinX;
                if (targetX > p2MaxX) targetX = p2MaxX;
            } else {
                targetX = winWidth - 80.0f;
            }

            float dx = targetX - p2.x;
            float ax = aiBaseSpeed * 0.7f;
            if (dx > ax)        p2.x += ax;
            else if (dx < -ax)  p2.x -= ax;
            else                p2.x = targetX;
        } else {
            if (specialDown[GLUT_KEY_UP])    p2.y += p2.speed;
            if (specialDown[GLUT_KEY_DOWN])  p2.y -= p2.speed;
            if (specialDown[GLUT_KEY_LEFT])  p2.x -= p2.speed;
            if (specialDown[GLUT_KEY_RIGHT]) p2.x += p2.speed;
        }

        // Clamp paddles
        float p1MinX = 40.0f;
        float p1MaxX = winWidth / 2.0f - 60.0f;
        float p2MinX = winWidth / 2.0f + 60.0f;
        float p2MaxX = winWidth - 40.0f;

        if (p1.x < p1MinX) p1.x = p1MinX;
        if (p1.x > p1MaxX) p1.x = p1MaxX;
        if (p2.x < p2MinX) p2.x = p2MinX;
        if (p2.x > p2MaxX) p2.x = p2MaxX;

        if (p1.y < p1.height/2)              p1.y = p1.height/2;
        if (p1.y > winHeight - p1.height/2)  p1.y = winHeight - p1.height/2;
        if (p2.y < p2.height/2)              p2.y = p2.height/2;
        if (p2.y > winHeight - p2.height/2)  p2.y = winHeight - p2.height/2;

        // Ball movement
        prevBallX = ball.x;
        prevBallY = ball.y;

        ball.x += ball.vx * speedFactor;
        ball.y += ball.vy * speedFactor;

        if (ball.y < ball.radius) {
            ball.y = ball.radius;
            ball.vy *= -1.0f;
        } else if (ball.y > winHeight - ball.radius) {
            ball.y = winHeight - ball.radius;
            ball.vy *= -1.0f;
        }

        // Left paddle collision
        {
            float px = p1.x;
            float py = p1.y;
            float pw = p1.width;
            float ph = p1.height;

            bool overlapY =
                (ball.y + ball.radius >= py - ph/2) &&
                (ball.y - ball.radius <= py + ph/2);

            float paddleRight = px + pw/2;

            bool crossingX =
                (prevBallX - ball.radius >= paddleRight && ball.x - ball.radius <= paddleRight) ||
                (ball.x - ball.radius <= paddleRight && ball.x - ball.radius >= px - pw/2);

            if (overlapY && crossingX) {
                ball.x = paddleRight + ball.radius;
                ball.vx = std::fabs(ball.vx);

                float offset = (ball.y - py) / (ph * 0.5f);
                ball.vy += offset * 1.5f;

                if (speedFactor < 2.0f) speedFactor += 0.05f;
                hitsInRally++;

            }
        }

        // Right paddle collision
        {
            float px = p2.x;
            float py = p2.y;
            float pw = p2.width;
            float ph = p2.height;

            bool overlapY =
                (ball.y + ball.radius >= py - ph/2) &&
                (ball.y - ball.radius <= py + ph/2);

            float paddleLeft = px - pw/2;

            bool crossingX =
                (prevBallX + ball.radius <= paddleLeft && ball.x + ball.radius >= paddleLeft) ||
                (ball.x + ball.radius >= paddleLeft && ball.x + ball.radius <= px + pw/2);

            if (overlapY && crossingX) {
                ball.x = paddleLeft - ball.radius;
                ball.vx = -std::fabs(ball.vx);

                float offset = (ball.y - py) / (ph * 0.5f);
                ball.vy += offset * 1.5f;

                if (speedFactor < 2.0f) speedFactor += 0.05f;
                hitsInRally++;


            }
        }

        // Goals
        if (ball.x < 0.0f) {
            scoreP2++;
            resetBall();
            flashFrames = 10;
            flashR = 1.0f; flashG = 0.2f; flashB = 0.2f;   // red flash
             // trigger shake
                shakeFrames    = 6;
                shakeIntensity = 3.0f * speedFactor;
        }
        if (ball.x > (float)winWidth) {
            scoreP1++;
            resetBall();
            flashFrames = 10;
            flashR = 0.2f; flashG = 0.5f; flashB = 1.0f;   // blue flash
             // trigger shake
                shakeFrames    = 6;
                shakeIntensity = 3.0f * speedFactor;
        }

        // Timer
        timeLeft -= 0.016f;
        if (timeLeft <= 0.0f) {
            timeLeft = 0.0f;
            currentState = STATE_GAME_OVER;
            stopBackgroundMusic();      // 🔇 stop when match ends by time
        }

        // Max score (0 = infinite)
        if (maxScore > 0 && (scoreP1 >= maxScore || scoreP2 >= maxScore)) {
            currentState = STATE_GAME_OVER;
            stopBackgroundMusic();      // 🔇 stop when someone reaches max score
        }
    }

    // dec flash & shake
    if (flashFrames > 0) flashFrames--;
    if (shakeFrames > 0) shakeFrames--;

    glutPostRedisplay();
    glutTimerFunc(16, timerCallback, 0);
}

// ===================== MAIN =====================

int main(int argc, char** argv) {
    srand((unsigned)time(0));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    int screenW = glutGet(GLUT_SCREEN_WIDTH);
    int screenH = glutGet(GLUT_SCREEN_HEIGHT);
    glutInitWindowSize(screenW, screenH);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("Paddle Rivals");

#ifdef _WIN32
    // 👇 Set custom window icon (taskbar + title bar)

    // 1) Get the HWND of the GLUT window by its title
    HWND hwnd = FindWindowA(NULL, "Paddle Rivals");

    // 2) Load the .ico file from disk
    HICON hIcon = (HICON)LoadImageA(
        NULL,
        "D:\\Prog\\C++\\Graphics\\Paddle Rivals\\icon.ico",
        IMAGE_ICON,
        0, 0,
        LR_LOADFROMFILE | LR_DEFAULTSIZE
    );

    if (hwnd && hIcon) {
        SendMessage(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }
#endif

    //glutFullScreen();

    glutDisplayFunc(displayCallback);
    glutReshapeFunc(reshapeCallback);
    glutKeyboardFunc(keyboardCallback);
    glutKeyboardUpFunc(keyboardUpCallback);
    glutSpecialFunc(specialCallback);
    glutSpecialUpFunc(specialUpCallback);
    glutTimerFunc(16, timerCallback, 0);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glutMainLoop();
    return 0;
}

