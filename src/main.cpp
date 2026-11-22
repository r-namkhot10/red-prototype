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

	float dot(const Vec2& other) const {
		return x * other.x + y * other.y;
	}

    Vec2 vecRotation(float angle) const {
        float rad = angle * DEG2RAD;
        float cosA = cos(rad);
        float sinA = sin(rad);
        return Vec2(x * cosA - y * sinA, x * sinA + y * cosA).normalize();
    }
};

Vec2 facingDir (Vec2& direction, Vec2& position, float speed, float dt, Vec2& facingdir) {
    direction = direction.length() > 0.0f ? direction.normalize() : direction;
    position = position + direction * speed * dt;
    if (direction.length() > 0.0f) {
        facingdir = direction;
    }
    return facingdir;
}

void clampPosition(float world_W, float world_H, float target_SIZE, float world_thick, Vec2& targetPos) {
    if (targetPos.x < world_thick + target_SIZE / 2.0f) targetPos.x = world_thick + target_SIZE / 2.0f;
    if (targetPos.y < world_thick + target_SIZE / 2.0f) targetPos.y = world_thick + target_SIZE / 2.0f;
    if (targetPos.x > world_W - world_thick - target_SIZE / 2.0f) targetPos.x = world_W - world_thick - target_SIZE / 2.0f;
    if (targetPos.y > world_H - world_thick - target_SIZE / 2.0f) targetPos.y = world_H - world_thick - target_SIZE / 2.0f;
}

void clampOverlapPosition(float player_SIZE, float enemy_SIZE, Vec2& playerPos, Vec2& enemyPos) {
    if (playerPos.distanceTo(enemyPos) < (player_SIZE / 2.0f + enemy_SIZE / 2.0f)) {
        Vec2 dir = (playerPos - enemyPos).normalize();
        float overlap = (player_SIZE / 2.0f + enemy_SIZE / 2.0f) - playerPos.distanceTo(enemyPos);
        playerPos = playerPos + dir * overlap;
    }
}

bool enemyTurn(float dt,Vec2 startFacingDir, Vec2& enemyfacingDir, float& enemyElapsed, float enemyTurnDuration, float& enemyAngle, float enemyTotalAngle) {
    enemyElapsed += dt;

    float t = (enemyTurnDuration > 0.0f) ? (enemyElapsed / enemyTurnDuration) : 1.0f;
    if (t > 1.0f) t = 1.0f;

    enemyAngle = enemyTotalAngle * t;
    enemyfacingDir = startFacingDir.vecRotation(enemyAngle);

    if (enemyElapsed >= enemyTurnDuration) {
        return true;
    }
    return false;
}

bool enemyMove(Vec2& enemyPos, Vec2& enemyfacingDir, float enemySpeed, float dt, float& walkedDistance, float targetDistance) {
    Vec2 dir = enemyfacingDir.normalize();
    Vec2 prevPos = enemyPos;

    enemyPos = enemyPos + dir * enemySpeed * dt;
    walkedDistance += prevPos.distanceTo(enemyPos);

    if (walkedDistance >= targetDistance) {
        walkedDistance = targetDistance;
        return true;
    }
    return false;
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

    Vec2 playerfacingDir(0.0f, 1.0f);
    Vec2 playerPos(WORLD_W / 2.0f, WORLD_H / 2.0f);
    float speedPlayer = 300.0f;
	float DashSpeed = 600.0f;
	float attackReach = 80.0f;
	float attackHalfAngle = 50.0f;
	float attackDamage = 20.0f;

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

	bool AttackedEnemy = false;

	enum class NotifTimer {
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
		float health;

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

	struct Enemy {
		Vec2 position;
		Vec2 facingdir;
		Vec2 direction;
		float speed;
		float health;
		float Viewingdistance;

		float elapsed = 0.0f;
		float currentAngle = 0.0f;
	};

	Enemy enemy1 = { Vec2((WORLD_W / 2.0f) + 120.0f, WORLD_H / 2.0f), Vec2(0.0f, 1.0f), Vec2(0.0f, 1.0f), 120.0f, 60, 330.0f };

	struct enemy1Move {
        enum class enemy1State {
            IDLE,
            ACTIVE
        };

		enum class enemy1MoveState {
			P,
            P2,
            P3,
            P4
		};

		enum class enemy1StopState {
			STOP_P,
			STOP_P2,
			STOP_P3
		};
	};

    enemy1Move::enemy1State enemy1State = enemy1Move::enemy1State::IDLE;
	enemy1Move::enemy1MoveState enemy1MoveState = enemy1Move::enemy1MoveState::P;

    float enemy1stopDur = 0.0f;

    float enemy1stopDurP = 1.0f;
    float enemy1stopDurP2 = 1.0f;
    float enemy1stopDurP3 = 1.0f;
    float enemy1stopDurP4 = enemy1stopDur;

	float enemy1targetDistP = 360.0f;
    float enemy1targetDistP2 = 240.0f;
    float enemy1targetDistP3 = 240.0f;
	float enemy1targetDistP4 = 120.0f; //don't know distance yet

	float enemy1totalAngleP = 90.0f;
    float enemy1totalAngleP2 = 120.0f;
    float enemy1totalAngleP3 = 45.0f;
    float enemy1totalAngleP4 = 100.0f; //don't know Angle yet

    float enemy1turnDurP = 1.0f;
    float enemy1turnDurP2 = 2.0f;
    float enemy1turnDurP3 = 1.5f;
    float enemy1turnDurP4 = 1.5f;

    Vec2 enemy1TurnStartDir = enemy1.facingdir;
    float enemy1walkedDist = 0.0f;
    bool enemy1HasTurnStartDir = false;
    static bool enemy1TurnDone = false;

	Red::AttackState ATKState = Red::AttackState::IDLE;
	Red::DefenseState DEFState = Red::DefenseState::BLOCKING;

    NotifTimer notiTimer = NotifTimer::IDLE;
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
					playerfacingDir = dir;
				}
			}

            if (IsKeyPressed(KEY_J) && ATKState == Red::AttackState::IDLE && DTimer >= WindupTime + ActiveTime + RecoveryTime) {
                ATKState = Red::AttackState::WINDUP;
                ATKTimer = 0.0f;
                DTimer = 0.0f;
				AttackedEnemy = false;
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

		// enemy1 move logic
		bool enemy1Turned = false;
		bool enemy1Moved = false;

		if (enemy1State == enemy1Move::enemy1State::IDLE) {
			enemy1stopDur += dt;
			if (enemy1stopDur >= enemy1stopDurP && enemy1MoveState == enemy1Move::enemy1MoveState::P) {
				enemy1State = enemy1Move::enemy1State::ACTIVE;
			}
			else if (enemy1stopDur >= enemy1stopDurP2 && enemy1MoveState == enemy1Move::enemy1MoveState::P2) {
				enemy1State = enemy1Move::enemy1State::ACTIVE;
			}
			else if (enemy1stopDur >= enemy1stopDurP3 && enemy1MoveState == enemy1Move::enemy1MoveState::P3) {
				enemy1State = enemy1Move::enemy1State::ACTIVE;
			}
			else if (enemy1stopDur >= enemy1stopDurP4 && enemy1MoveState == enemy1Move::enemy1MoveState::P4) {
				enemy1State = enemy1Move::enemy1State::ACTIVE;
			}
		}

        if (enemy1State == enemy1Move::enemy1State::ACTIVE) {
            float targetDist = enemy1targetDistP;
			float enemy1turnDur = enemy1turnDurP;
			float enemy1totalAngle = enemy1totalAngleP;
            enemy1Move::enemy1MoveState nextMoveState = enemy1Move::enemy1MoveState::P2;

            if (enemy1MoveState == enemy1Move::enemy1MoveState::P) {
                targetDist = enemy1targetDistP;
                enemy1turnDur = enemy1turnDurP;
                enemy1totalAngle = enemy1totalAngleP;
                nextMoveState = enemy1Move::enemy1MoveState::P2;
            }
            else if (enemy1MoveState == enemy1Move::enemy1MoveState::P2) {
                targetDist = enemy1targetDistP2;
				enemy1turnDur = enemy1turnDurP2;
				enemy1totalAngle = enemy1totalAngleP2;
                nextMoveState = enemy1Move::enemy1MoveState::P3;
            }
            else if (enemy1MoveState == enemy1Move::enemy1MoveState::P3) {
                targetDist = enemy1targetDistP3;
                enemy1turnDur = enemy1turnDurP3;
                enemy1totalAngle = enemy1totalAngleP3;
                nextMoveState = enemy1Move::enemy1MoveState::P4;
            }
            else if (enemy1MoveState == enemy1Move::enemy1MoveState::P4) {
                targetDist = enemy1targetDistP4;
				enemy1turnDur = enemy1turnDurP4;
				enemy1totalAngle = enemy1totalAngleP4;
                nextMoveState = enemy1Move::enemy1MoveState::P;
            }

            if (!enemy1TurnDone) {
                if (!enemy1HasTurnStartDir) {
                    enemy1.elapsed = 0.0f;
                    enemy1TurnStartDir = enemy1.facingdir;
                    enemy1HasTurnStartDir = true;
                }
                if (enemyTurn(dt, enemy1TurnStartDir, enemy1.facingdir, enemy1.elapsed, enemy1turnDur, enemy1.currentAngle, enemy1totalAngle)) {
                    enemy1TurnDone = true;
                    enemy1walkedDist = 0.0f;
                }
            }
            else {
                if (enemyMove(enemy1.position, enemy1.facingdir, enemy1.speed, dt, enemy1walkedDist, targetDist)) {
                    enemy1TurnDone = false;
                    enemy1HasTurnStartDir = false;
                    enemy1walkedDist = 0.0f;
                    enemy1stopDur = 0.0f;
                    enemy1State = enemy1Move::enemy1State::IDLE;
                    enemy1MoveState = nextMoveState;
                }
            }
        }
        
        

        clampPosition(WORLD_W , WORLD_H , PLAYER_SIZE, WORLD_THICK, playerPos);
        clampPosition(WORLD_W , WORLD_H , ENEMY_SIZE, WORLD_THICK, enemy1.position);

		if (enemy1.health > 0) {
		    clampOverlapPosition(PLAYER_SIZE, ENEMY_SIZE, playerPos, enemy1.position);
		}

        Vec2 DirToEnemy = (enemy1.position - playerPos);
		float distanceToEnemy = DirToEnemy.length() - (ENEMY_SIZE / 2.0f);
		bool isEnemyInRange = distanceToEnemy <= attackReach;
        bool isEnemyInFront = playerfacingDir.dot(DirToEnemy.normalize()) > cos(attackHalfAngle * DEG2RAD);
		bool isEnemyAttackable = isEnemyInRange && isEnemyInFront;

		if (ATKState == Red::AttackState::ACTIVE && isEnemyAttackable && !AttackedEnemy && enemy1.health > 0) {
			enemy1.health -= attackDamage;
			AttackedEnemy = true;
		}

        camera.target = Vector2{ playerPos.x, playerPos.y };

		Vec2 playerEndPos = playerPos + playerfacingDir * PLAYER_SIZE / 2.0f;
		Vec2 enemy1EndPos = enemy1.position + enemy1.facingdir * ENEMY_SIZE / 2.0f;
        Vec2 enemy1VisionEndPos = enemy1.position + enemy1.facingdir.vecRotation(60) * enemy1.Viewingdistance;
        Vec2 enemy1VisionEndPos2 = enemy1.position + enemy1.facingdir.vecRotation(-60) * enemy1.Viewingdistance;

		float checkVisionDistance = enemy1VisionEndPos.distanceTo(enemy1.position);
		float checkVisionDistance2 = enemy1VisionEndPos2.distanceTo(enemy1.position);

		Color EnemyColor = PURPLE;
		if (ATKState == Red::AttackState::ACTIVE && isEnemyAttackable) {
            EnemyColor = GREEN;
		}

        BeginDrawing();
        ClearBackground(RAYWHITE);
        if (state == GameState::PLAYING) {
            BeginMode2D(camera);
            DrawRectangleLinesEx(Rectangle{ 0.0f, 0.0f, (float)WORLD_W, (float)WORLD_H}, (float)WORLD_THICK, DARKGRAY);


            if (enemy1.health > 0) {
                DrawRectangle((int)enemy1.position.x - ENEMY_SIZE / 2.0f, (int)enemy1.position.y - ENEMY_SIZE / 2.0f, ENEMY_SIZE, ENEMY_SIZE, EnemyColor);
                DrawLineEx(Vector2{ enemy1.position.x, enemy1.position.y }, Vector2{ enemy1VisionEndPos.x, enemy1VisionEndPos.y }, (ENEMY_SIZE / 30.0f), GRAY);
				DrawLineEx(Vector2{ enemy1.position.x, enemy1.position.y }, Vector2{ enemy1VisionEndPos2.x, enemy1VisionEndPos2.y }, (ENEMY_SIZE / 30.0f), GRAY);
				DrawLineEx(Vector2{ enemy1.position.x, enemy1.position.y }, Vector2{ enemy1EndPos.x, enemy1EndPos.y }, (ENEMY_SIZE / 10.0f), BLACK);

                DrawText(TextFormat("HP: %.0f", enemy1.health), (int)enemy1.position.x - ENEMY_SIZE / 2.0f, (int)enemy1.position.y - ENEMY_SIZE / 2.0f - 20, 20, BLACK);
                DrawText(TextFormat("( %.2f, %.2f )", enemy1.facingdir.x, enemy1.facingdir.y), (int)enemy1.position.x - ENEMY_SIZE / 2.0f, (int)enemy1.position.y - ENEMY_SIZE / 2.0f - 40, 20, BLACK);
                DrawText(TextFormat("Vision Dist: %.2f, %.2f", checkVisionDistance, checkVisionDistance2), (int)enemy1.position.x - ENEMY_SIZE / 2.0f, (int)enemy1.position.y - ENEMY_SIZE / 2.0f - 60, 20, BLACK);
            }

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
