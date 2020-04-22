#include <iostream>
#include <Windows.h>
#include <chrono>
#include <vector>
#include <stdio.h>
#include <utility>
#include <algorithm>

using namespace std;

int nScreenWidth = 120;
int nScreenHeight = 40;

float fPlayerX = 3.0F;
float fPlayerY = 3.0F;
float fPlayerA = 0.0F;

float fFOV = 3.14159F / 4.0F;
float fDepth = 16;

int nMapWidth = 16;
int nMapHeight = 16;

int main() {
	// Create Screen Buffer
	wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	wstring map;
	map += L"################";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#########......#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"################";


	auto tp1 = chrono::system_clock::now();
	auto tp2 = chrono::system_clock::now();

	// Game Loop
	while (1) {

		tp2 = chrono::system_clock::now();
		chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();

		// Controls
		// Handle CCW Rotation
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
			fPlayerA -= (0.8F) * fElapsedTime;
		
		if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
			fPlayerA += (0.8F) * fElapsedTime;

		if (GetAsyncKeyState((unsigned short)'W') & 0x8000) {

			fPlayerX += sinf(fPlayerA) * 5.0F * fElapsedTime;
			fPlayerY += cosf(fPlayerA) * 5.0F * fElapsedTime;

			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
				fPlayerX -= sinf(fPlayerA) * 5.0F * fElapsedTime;
				fPlayerY -= cosf(fPlayerA) * 5.0F * fElapsedTime;
			}
		}

		if (GetAsyncKeyState((unsigned short)'S') & 0x8000) {
			fPlayerX -= sinf(fPlayerA) * 5.0F * fElapsedTime;
			fPlayerY -= cosf(fPlayerA) * 5.0F * fElapsedTime;

			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
				fPlayerX += sinf(fPlayerA) * 5.0F * fElapsedTime;
				fPlayerY += cosf(fPlayerA) * 5.0F * fElapsedTime;
			}
		}

		for (int x = 0; x < nScreenWidth; x++) {

			// For each column, calculate the projected ray angle into the world space
			float fRayAngle = (fPlayerA - fFOV / 2.0F) + ((float)x / (float)nScreenWidth) * fFOV;
			float fDistanceToWall = 0;

			bool bHitWall = false;
			bool bBoundary = false;

			float fEyeX = sinf(fRayAngle); // Unit vector for ray in player space
			float fEyeY = cosf(fRayAngle);

			while (!bHitWall && fDistanceToWall < fDepth) {

				fDistanceToWall += 0.01F;

				int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				// Test if ray is out of bounds
				if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight) {
					bHitWall = true; // just set distance to maximum depth
					fDistanceToWall = fDepth;
				} else {

					// Ray is inbounds so test to see if the ray cell is a wall block
					if (map[nTestY * nMapWidth + nTestX] == '#') {
						bHitWall = true;
	
						vector<pair<float, float>> p; // distance, dot

						for (int tX = 0; tX < 2; tX++) {
							for (int tY = 0; tY < 2; tY++) {
								float vY = (float)nTestY + tY - fPlayerY;
								float vX = (float)nTestX + tX - fPlayerX;

								float d = sqrt(vX * vX + vY * vY);
								float dot = (fEyeX * vX / d) + (fEyeY * vY / d);
								p.push_back(make_pair(d, dot));
							}
						}
						
						// Sort Pairs from closest to farthest
						sort(p.begin(), p.end(), [](const pair<float, float>&left, const pair<float, float>&right) {return left.first < right.first; });

						float fBound = 0.01;

						if (acos(p.at(0).second) < fBound) bBoundary = true;
						if (acos(p.at(1).second) < fBound) bBoundary = true;

					}
				}
			}

			// Calculate distance to ceiling and floor
			float nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
			float nFloor = nScreenHeight - nCeiling;

			short nShade = ' ';

			if (fDistanceToWall <= fDepth / 4.0F) nShade = 0x2588; // Very Close
			else if (fDistanceToWall < fDepth / 3.0F) nShade = 0x2593;
			else if (fDistanceToWall < fDepth / 2.0F) nShade = 0x2592;
			else if (fDistanceToWall < fDepth) nShade = 0x2591;
			else nShade = ' '; // Too far away

			if (bBoundary) nShade = ' ';

			for (int y = 0; y < nScreenHeight; y++) {
				if (y < nCeiling) screen[y * nScreenWidth + x] = ' ';
				else if (y > nCeiling && y <= nFloor) screen[y * nScreenWidth + x] = nShade;
				else {
					// Shade floor based on distance
					float b = 1.0F - (((float)y - nScreenHeight / 2.0F) / ((float)nScreenHeight / 2.0F));
					if (b < 0.25) nShade = '#';
					else if (b < 0.5) nShade = 'x';
					else if (b < 0.75) nShade = '.';
					else if (b < 0.9) nShade = '-';
					else nShade = ' ';

					screen[y * nScreenWidth + x] = nShade;
				}
			} 

		}

		// Display Stats
		swprintf_s(screen, 40, L"X=%3.2F, Y=%3.2F, A=%3.2F FPS=%3.2F ", fPlayerX, fPlayerY, fPlayerA, 1.0F / fElapsedTime);

		// Display Map
		for (int nX = 0; nX < nMapWidth; nX++)
			for (int nY = 0; nY < nMapHeight; nY++) {
				screen[(nY + 1) * nScreenWidth + nX] = map[nY * nMapWidth + nX];
			}

		screen[((int)fPlayerY + 1) * nScreenWidth + (int)fPlayerX] = 'P';

		//screen[nScreenWidth * nScreenHeight - 1] = '\0';
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0, 0 }, &dwBytesWritten);
	}


	return 0;
}
