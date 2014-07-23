#pragma once
#include "Local.h"
#include "Player.h"

class Client {
private:
	// Viewport
	Font* ptConsolasFont;
	void DrawViewportInfo();

	// FPS
#define FRAME_CAPTURE	20				// Capture 20 frames worth of activity for FPS average
	unsigned int frametimes[FRAME_CAPTURE];
	unsigned int framecount;
	unsigned int framelast;
	float fps;
	unsigned int frametime;
	void RunFPS();

	// HUD
	Menu* ptHUD;
public:
	Client();
	~Client();

	void Frame();
	void Preframe();
	unsigned int GetFrametime() { return frametime; }

	void HideLabels();
	void StartLabelDraw(const char* label);
	void EnteredArea(const char* area);
	bool bShouldDrawLabels;
	bool bStoppedDrawingLabels;
	bool bEntDrawingLabels; // An entity told us to draw labels, so draw it, damn it.
	Entity* ptFocusEnt;

	void PassMouseUp(int x, int y);
	void PassMouseDown(int x, int y);
	void PassMouseMove(int x, int y);

	unsigned int playerNum;
	int cursorX;
	int cursorY;

	int screenWidth;
	int screenHeight;

	Player* ptPlayer;
};

extern Client* thisClient;