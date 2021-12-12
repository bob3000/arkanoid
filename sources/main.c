#include "raylib.h"
#include <string.h>

#define SCREEN_WIDTH (800)
#define SCREEN_HEIGHT (450)

#define WINDOW_TITLE "Window title"

#define PADDLE_VELOCITY 4.0f
#define PADDLE_HEIGHT 20.0f
#define PADDLE_WIDTH 80.0f
#define BRICK_HEIGHT 30.0f
#define BRICK_WIDTH 60.0f
#define BALL_RADIUS 10.0f
#define BALL_VELOCITY 2.0f
#define WALL_WIDTH 12
#define WALL_HEIGHT 6

//------------------------------------------------------------
// Game Objects
//------------------------------------------------------------

typedef enum {
  BRICK,
  PADDLE,
} ObjType;

// Bricks
typedef struct {
  Vector2 position;
  Vector2 dimensions;
  Color color;
  unsigned int health;
  ObjType type;
} Brick;

typedef struct {
  unsigned int height;
  unsigned int width;
  Brick *bricks;
} Wall;

Wall *WallBuild(unsigned int height, unsigned int width) {
  Brick *bricks = (Brick *)MemAlloc(sizeof(Brick) * (height * width));
  bool even = false;
  float startPos = (SCREEN_WIDTH - (width * BRICK_WIDTH)) / 2.0;
  Vector2 position = {startPos, 0.0};
  for (int i = 0; i < width * height; i++) {
    if (i % width == 0) {
      even = !even;
      position.x = startPos;
      position.y += BRICK_HEIGHT;
    } else {
      position.x += BRICK_WIDTH;
    }

    bricks[i] = (Brick){.position = {position.x, position.y},
                        .dimensions = {BRICK_WIDTH, BRICK_HEIGHT},
                        .color = GREEN,
                        .health = 1,
                        .type = BRICK};

    if (even) {
      bricks[i].color = LIGHTGRAY;
    }
  }
  Wall *wall = (Wall *)MemAlloc(sizeof(Wall));
  wall->height = height;
  wall->width = width;
  wall->bricks = bricks;
  return wall;
}

void WallDestroy(Wall *wall) { MemFree(wall); }

void WallRender(Wall *wall) {
  for (int i = 0; i < wall->width * wall->height; i++) {
    if (wall->bricks[i].health <= 0)
      continue;
    DrawRectangle(wall->bricks[i].position.x, wall->bricks[i].position.y,
                  wall->bricks[i].dimensions.x, wall->bricks[i].dimensions.y,
                  wall->bricks[i].color);
  }
}

int WallBrickCount(Wall *wall) { return wall->height * wall->width; }

int WallBrickActiveCount(Wall *wall) {
  int activeCount = 0;
  for (int i = 0; i < WallBrickCount(wall); i++) {
    if (wall->bricks[i].health > 0) {
      activeCount += 1;
    }
  }
  return activeCount;
}

// Paddle
typedef struct {
  Vector2 position;
  Vector2 dimensions;
  Color color;
  ObjType type;
} Paddle;

Paddle *PaddleNew() {
  Paddle *paddle = (Paddle *)MemAlloc(sizeof(Paddle));
  paddle->position =
      (Vector2){(float)SCREEN_WIDTH / 2, (float)SCREEN_HEIGHT - PADDLE_HEIGHT};
  paddle->dimensions = (Vector2){PADDLE_WIDTH, PADDLE_HEIGHT};
  paddle->color = GREEN;
  paddle->type = PADDLE;
  return paddle;
}

void PaddleDestroy(Paddle *paddle) { MemFree(paddle); }

void PaddleRender(Paddle *paddle) {
  DrawRectangle(paddle->position.x, paddle->position.y, paddle->dimensions.x,
                paddle->dimensions.y, paddle->color);
}

void PaddleMove(Paddle *paddle) {
  if (IsKeyDown(KEY_RIGHT) &&
      ((paddle->position.x + paddle->dimensions.x) <= SCREEN_WIDTH))
    paddle->position.x += PADDLE_VELOCITY;
  if (IsKeyDown(KEY_LEFT) && ((paddle->position.x) >= 0))
    paddle->position.x -= PADDLE_VELOCITY;
}

// Ball
typedef struct {
  bool active;
  Vector2 position;
  Vector2 velocity;
  Color color;
} Ball;

Ball *BallNew() {
  Ball *ball = (Ball *)MemAlloc(sizeof(Ball));
  ball->active = true;
  ball->color = GREEN;
  ball->velocity = (Vector2){BALL_VELOCITY, BALL_VELOCITY};
  ball->position =
      (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT * 7.0f / 8 - 30};
  return ball;
}

void BallDestroy(Ball *ball) { MemFree(ball); }
void BallRender(Ball *ball) {
  if (!ball->active)
    return;
  DrawCircleV(ball->position, BALL_RADIUS, ball->color);
}
void BallMove(Ball *ball) {
  if (!ball->active)
    return;
  ball->position.x += ball->velocity.x;
  ball->position.y += ball->velocity.y;
  if ((ball->position.x + BALL_RADIUS) >= SCREEN_WIDTH) {
    ball->velocity.x *= -1;
  }
  if ((ball->position.x - BALL_RADIUS) <= 0) {
    ball->velocity.x *= -1;
  }
  if ((ball->position.y - BALL_RADIUS) <= 0) {
    ball->velocity.y *= -1;
  }
  if ((ball->position.y) >= SCREEN_HEIGHT) {
    ball->active = false;
  }
}

bool BallHandleCollision(Ball *ball, void *other, ObjType type) {
  switch (type) {
  case BRICK: {
    Brick *brick = (Brick *)other;
    Rectangle r = {.x = brick->position.x,
                   .y = brick->position.y,
                   .width = brick->dimensions.x,
                   .height = brick->dimensions.y};
    if (CheckCollisionCircleRec(ball->position, BALL_RADIUS, r)) {
      brick->health -= 1;
      ball->velocity.y *= -1;
      return true;
    }
    break;
  }
  case PADDLE: {
    Paddle *paddle = (Paddle *)other;
    Rectangle r = {.x = paddle->position.x,
                   .y = paddle->position.y,
                   .width = paddle->dimensions.x,
                   .height = paddle->dimensions.y};
    if (CheckCollisionCircleRec(ball->position, BALL_RADIUS, r)) {
      ball->velocity.y *= -1;
      return true;
    }
    break;
  }
  default: {
    TraceLog(LOG_ERROR, "unhandled ObjType %s in collision detection", type);
  }
  }
  return false;
}

// Text
typedef enum {
  LEFT,
  CENTER,
  RIGHT,
} Align;

typedef struct {
  int fontSize;
  int textLen;
  Color color;
  int posY;
  Align align;
  char *body;
} TextLine;

TextLine *TextLineNew(const char *body, int posY, Align align, int fontSize,
                      Color color) {
  TextLine *textLine = (TextLine *)MemAlloc(sizeof(TextLine));
  textLine->fontSize = fontSize;
  textLine->textLen = MeasureText(body, fontSize);
  textLine->posY = posY;
  textLine->color = color;
  textLine->align = align;
  textLine->body = (char *)MemAlloc(strlen(body) + 1);
  strncpy(textLine->body, body, strlen(body));
  return textLine;
}

void TextLineRender(TextLine *textLine) {
  int posX;
  switch (textLine->align) {
  case LEFT: {
    posX = 0;
    break;
  }
  case CENTER: {
    posX = (SCREEN_WIDTH / 2) - (textLine->textLen / 2);
    break;
  }
  case RIGHT: {
    posX = SCREEN_WIDTH - textLine->textLen;
    break;
  }
  }
  DrawText(textLine->body, posX, textLine->posY, textLine->fontSize, GREEN);
}

void TextLineDestroy(TextLine *textLine) { MemFree(textLine); }

//------------------------------------------------------------//
// Game Flow
//------------------------------------------------------------//

int main(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);

  // Initialize Objects
  bool gamePaused = true;
  Paddle *paddle = PaddleNew();
  Wall *wall = WallBuild(WALL_HEIGHT, WALL_WIDTH);
  Ball *ball = BallNew();

  TextLine *textGamePaused = TextLineNew("GAME PAUSED", 40, CENTER, 40, GREEN);
  TextLine *textGameOver = TextLineNew("GAME OVER", 40, CENTER, 40, GREEN);
  TextLine *textGameResume =
      TextLineNew("Press P to resume game", 120, CENTER, 20, GREEN);
  TextLine *textGameReset =
      TextLineNew("Press R to reset game", 140, CENTER, 20, GREEN);
  TextLine *textGameQuit =
      TextLineNew("Press Q to quit game", 160, CENTER, 20, GREEN);

  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    // Control keys
    if (IsKeyDown(KEY_Q)) {
      break;
    }
    if (IsKeyDown(KEY_P)) {
      gamePaused = !gamePaused;
    }
    if (IsKeyDown(KEY_R)) {
      paddle = PaddleNew();
      wall = WallBuild(WALL_HEIGHT, WALL_WIDTH);
      ball = BallNew();
    }

    if (!ball->active) {
      BeginDrawing();
      ClearBackground(BLACK);
      TextLineRender(textGameOver);
      TextLineRender(textGameReset);
      TextLineRender(textGameQuit);
      EndDrawing();
      continue;
    }

    if (gamePaused) {
      BeginDrawing();
      ClearBackground(BLACK);
      TextLineRender(textGamePaused);
      TextLineRender(textGameResume);
      TextLineRender(textGameReset);
      TextLineRender(textGameQuit);
      EndDrawing();
      continue;
    }

    // Update
    PaddleMove(paddle);
    BallMove(ball);
    if (!BallHandleCollision(ball, paddle, PADDLE)) {
      for (int i = 0; i < WallBrickCount(wall); i++) {
        if ((wall->bricks[i].health > 0) &&
            BallHandleCollision(ball, &wall->bricks[i], BRICK)) {
          break;
        }
      }
    }

    // Render
    BeginDrawing();

    ClearBackground(BLACK);

    WallRender(wall);
    BallRender(ball);
    PaddleRender(paddle);

    EndDrawing();
  }

  WallDestroy(wall);
  PaddleDestroy(paddle);
  BallDestroy(ball);
  TextLineDestroy(textGamePaused);
  TextLineDestroy(textGameResume);
  TextLineDestroy(textGameReset);
  TextLineDestroy(textGameQuit);
  TextLineDestroy(textGameOver);
  CloseWindow();

  return 0;
}
