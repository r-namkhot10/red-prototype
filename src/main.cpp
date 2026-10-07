#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include "raylib.h"
#include "resource_dir.h"

struct Vec2
{
    float x, y;

    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& other) const {
        return { x + other.x, y + other.y };
    }

    Vec2 operator-(const Vec2& other) const {
        return { x - other.x, y - other.y };
    }

    Vec2 operator*(float k) const {
        return { x * k, y * k };
    }

    Vec2 operator/(float k) const {
        return { x / k, y / k };
    }

    float length() const {
        return std::sqrt(x * x + y * y);
    }

    Vec2 normalize() const {
        float len = length();
        if (len == 0.0f)
            return { 0.0f, 0.0f };
        return { x / len, y / len };
    }

    float distanceTo(const Vec2& other) const {
        return (*this - other).length();
    }
};

void clampPosition(float world_W, float world_H, float TARGET_SIZE, float world_thick, Vec2& position) {
    if (position.x < world_thick + TARGET_SIZE / 2.0f) position.x = world_thick + TARGET_SIZE / 2.0f;
    if (position.y < world_thick + TARGET_SIZE / 2.0f) position.y = world_thick + TARGET_SIZE / 2.0f;
    if (position.x > world_W - world_thick - TARGET_SIZE / 2.0f) position.x = world_W - world_thick - TARGET_SIZE / 2.0f;
    if (position.y > world_H - world_thick - TARGET_SIZE / 2.0f) position.y = world_H - world_thick - TARGET_SIZE / 2.0f;
}

int main()
{
    const int SCREEN_W = 1500;
    const int SCREEN_H = 900;
    const int WORLD_W = 2000;
    const int WORLD_H = 2000;
	const int WORLD_THICK = 20;
    const int PLAYER_SIZE = 40;
    const int ENEMY_SIZE = 50;

    InitWindow(SCREEN_W, SCREEN_H, "red");
    SetTargetFPS(60);

    Vec2 facingDir(0.0f, 1.0f);
    Vec2 playerPos(WORLD_W / 2.0f, WORLD_H / 2.0f);
    float speedPlayer = 300.0f;
	float DashSpeed = 600.0f;

    Camera2D camera = {};
    camera.offset = Vector2{ SCREEN_W / 2.0f, SCREEN_H / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.5f;

    float timer = 0.0f;
	float NTimer = 0.0f;
	float ATKTimer = 0.0f;

	float WindupTime = 0.12f;
	float ActiveTime = 0.08f;
	float RecoveryTime = 0.30f;
	float CancelTimeFrame = 0.3f;

	float speedP_Windup = speedPlayer * 0.3f;
	float speedP_Active = 0.0f;
	float speedP_Recovery = speedPlayer * 0.5f;

	float DTimer = WindupTime + ActiveTime + RecoveryTime;

	enum class NotificationTimer {
		IDLE,
		ACTIVE
	};

    enum class GameState {
        WAIT_FOR_START,
        PLAYING,
        GAME_OVER,
        WIN
    };

	struct Red {
		Vec2 position;
		float speed;
		int health;

        enum class AttackState {
            IDLE,
            WINDUP,
            ACTIVE,
            RECOVERY
        };

		enum class DefenseState {
            IDLE,
			BLOCKING,
			DODGING
		};

	};

	Red::AttackState ATKState = Red::AttackState::IDLE;
	Red::DefenseState DEFState = Red::DefenseState::BLOCKING;

    NotificationTimer notificationTimer = NotificationTimer::IDLE;
    GameState state = GameState::WAIT_FOR_START;

    while (state == GameState::WAIT_FOR_START && !WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        if (IsKeyPressed(KEY_ENTER)) { state = GameState::PLAYING; }
        DrawText("Welcome to the Red Prototype Game!\n"
			"????????.\n" //-----
            "***How to Play***\n"
            "Controls: ???\n" //-----
            "Player starts at the center of the world.\n", 40, 120, 20, BLACK);
        DrawText("Press Enter to start...", 40, 300, 20, BLACK);
        EndDrawing();
    }

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        float speedPT = speedPlayer;

        if (state == GameState::PLAYING) {
            timer += dt;

			if (ATKState == Red::AttackState::WINDUP) {
				speedPT = speedP_Windup;
			}
			else if (ATKState == Red::AttackState::ACTIVE) {
				speedPT = speedP_Active;
			}
			else if (ATKState == Red::AttackState::RECOVERY) {
				speedPT = speedP_Recovery;
			}

            Vec2 dir(0.0f, 0.0f);

                if (IsKeyDown(KEY_D)) dir.x += 1.0f;
                if (IsKeyDown(KEY_A)) dir.x -= 1.0f;
                if (IsKeyDown(KEY_S)) dir.y += 1.0f;
                if (IsKeyDown(KEY_W)) dir.y -= 1.0f;

                dir = dir.normalize();
                playerPos = playerPos + dir * speedPT * dt;

            if (ATKState == Red::AttackState::IDLE) {
				if (dir.length() > 0.0f) {
					facingDir = dir;
				}
			}

            if (IsKeyPressed(KEY_J) && ATKState == Red::AttackState::IDLE && DTimer >= WindupTime + ActiveTime + RecoveryTime) {
                ATKState = Red::AttackState::WINDUP;
                ATKTimer = 0.0f;
                DTimer = 0.0f;
            }

            if (DTimer < WindupTime + ActiveTime + RecoveryTime) {
                DTimer += dt;
            }

            if (ATKState != Red::AttackState::IDLE) {
				ATKTimer += dt;

                if (IsKeyPressed(KEY_SPACE) && DTimer <= CancelTimeFrame) {
					ATKState = Red::AttackState::IDLE;
                    ATKTimer = 0.0f;
                }
				else if (ATKState == Red::AttackState::WINDUP) {
					if (ATKTimer >= WindupTime) {
						ATKState = Red::AttackState::ACTIVE;
						ATKTimer = 0.0f;
					}
				}
				else if (ATKState == Red::AttackState::ACTIVE) {
					if (ATKTimer >= ActiveTime) {
						ATKState = Red::AttackState::RECOVERY;
						ATKTimer = 0.0f;
					}
				}
				else if (ATKState == Red::AttackState::RECOVERY) {
					if (ATKTimer >= RecoveryTime) {
						ATKState = Red::AttackState::IDLE;
						ATKTimer = 0.0f;
					}
				}
            }

            if (IsKeyPressed(KEY_SPACE) && ATKState == Red::AttackState::IDLE) {
                playerPos = playerPos + dir * DashSpeed * dt;
                ATKState = Red::AttackState::IDLE;
                ATKTimer = 0.0f;
            }

        }

        if (IsKeyDown(KEY_Q)) break;

        clampPosition(WORLD_W , WORLD_H , PLAYER_SIZE, WORLD_THICK, playerPos);
 
        camera.target = Vector2{ playerPos.x, playerPos.y };
        
		Vec2 playerEndPos = playerPos + facingDir * PLAYER_SIZE / 2.0f;

        BeginDrawing();
        ClearBackground(RAYWHITE);
        if (state == GameState::PLAYING) {
            BeginMode2D(camera);
            DrawRectangleLinesEx(Rectangle{ 0.0f, 0.0f, (float)WORLD_W, (float)WORLD_H}, (float)WORLD_THICK, DARKGRAY);
            DrawRectangle((int)playerPos.x - PLAYER_SIZE / 2.0f, (int)playerPos.y - PLAYER_SIZE / 2.0f, PLAYER_SIZE, PLAYER_SIZE, RED);
			DrawLineEx(Vector2{playerPos.x, playerPos.y}, Vector2{playerEndPos.x, playerEndPos.y}, (PLAYER_SIZE / 10.0f), BLACK);
            EndMode2D();
        }

        else if (state == GameState::GAME_OVER) {
            DrawText("*** GAME OVER ***\n", 40, 40, 60, BLACK); //------
        }

        else if (state == GameState::WIN) {
            DrawText("\n*** YOU WIN! ***\n", 40, 40, 60, BLACK); //-------
        }

        EndDrawing();
    }

    CloseWindow();
}
