🎮 Paddle Rivals – The Ultimate Arcade Showdown
<p align="center"> <img src="Assets/logo.png" width="200" alt="Paddle Rivals Logo"/> </p> <p align="center"> <b>⚡ Neon. ⚔️ Rivalry. 🕹️ Precision.</b><br> A modern OpenGL-powered Pong remake with dynamic menus, AI opponents, FX, and theme customization. </p>
🚀 Overview

Paddle Rivals is a high-energy arcade Pong game built entirely in C++ & OpenGL (FreeGLUT).

Featuring:

Full menu system

Single & Multiplayer modes

Adjustable AI difficulty

Themes, background FX, screen shake, and scoring flashes

Avatar selection, player names, and polished UI

Background music + custom game icon

A touch of 3D spice (rotating cube)

Built from scratch in one night like legends 😤🔥

⭐ Key Features
🎮 Gameplay

⚔️ Single Player (AI with Easy / Medium / Hard)

🤝 Multiplayer (1v1 keyboard)

🧊 Smooth 4-direction paddle control

🏐 Speed-scaling ball physics

💥 Flash + screen shake when scoring

🔄 Pause menu with resume / menu options

🧠 AI System

Smooth tracking of ball movement

Difficulty affects speed & reaction time

Moves on both X and Y axes

No stutter, no jitter

🎨 Visuals & Themes

Choose between 3 animated themes:

🌌 Cosmic Field

🌃 Neon Night

🕹️ Retro Grid (default)

Extras:

3D spinning cube

Ball glow effect

Themed HUD

Smooth gradients and grids

🔊 Audio & Polish

Always-on looped background music (bg_music.wav)

Custom window icon (icon.ico)

Fake-fullscreen immersive window for gameplay feel

📂 Project Structure
Paddle-Rivals/
│
├── Assets/
│   ├── logo.png
│   ├── icon.ico
│   ├── bg_music.wav
│   ├── screenshots/
│   │     ├── menu.png
│   │     ├── gameplay.png
│   │     ├── avatars.png
│   │     └── settings.png
│
├── Demo_Video/
│   └── demo.mp4
│
├── main.cpp
└── README.md

🎥 Demo & Screenshots
📹 Full Gameplay Demo

▶️ Watch Demo Video

🖼️ Screenshots
Main Menu	In-Game
<img src="Assets/screenshots/menu.png" width="420"/>	<img src="Assets/screenshots/gameplay.png" width="420"/>
Avatar Select	Settings
<img src="Assets/screenshots/avatars.png" width="420"/>	<img src="Assets/screenshots/settings.png" width="420"/>
🎮 Controls
Single Player
Action	Keys
Move	W / A / S / D or Arrow Keys
Pause	ESC
Multiplayer
Player	Up	Down	Left	Right
P1	W	S	A	D
P2	↑	↓	←	→
⚙️ Building the Game (Windows – CodeBlocks)
1️⃣ Install Dependencies

Download freeglut binaries:

freeglut.dll

freeglut.lib

opengl32.lib

glu32.lib

2️⃣ Link Libraries

In Build Options → Linker Settings, add:

-lfreeglut
-lopengl32
-lglu32


Put freeglut.dll next to your .exe inside bin/Debug.

🖼️ Adding the Custom Icon

Place icon.ico inside Assets/ and include:

#ifdef _WIN32
HANDLE hIcon = LoadImage(NULL, "Assets/icon.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
SendMessage(GetActiveWindow(), WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
SendMessage(GetActiveWindow(), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
#endif

🔊 Adding Background Music

Place bg_music.wav inside Assets/.

Add:

PlaySound("Assets/bg_music.wav", NULL, SND_LOOP | SND_ASYNC);

👑 Credits

Developed by:

Mohamed Abdallah Eldairouty (@MohamedEldairouty)

Course:
Computer Graphics – AAST 2025

📝 License

This project is for academic use and portfolio showcasing.
© 2025 Paddle Rivals Team.
