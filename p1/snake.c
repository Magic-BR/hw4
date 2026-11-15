#include "snake.h"

#include <stdlib.h>

/* ------------------------------------------------------------------------- */
/* Direction helpers                                                          */
/* ------------------------------------------------------------------------- */

/* Reverse of a direction; used to reject 180-degree turns. */
static Direction opposite(Direction dir) {
    switch (dir) {
        case DIR_RIGHT:
            return DIR_LEFT;
        case DIR_LEFT:
            return DIR_RIGHT;
        case DIR_DOWN:
            return DIR_UP;
        case DIR_UP:
            return DIR_DOWN;
        default:
            return DIRECTION_COUNT;
    }
}

/*
 * Resolve the requested turn against the current heading. A direct reverse
 * is ignored while the snake has more than one node; at length 1 it is kept.
 * Invalid request values are ignored and the current heading is retained.
 */
static Direction effectiveDirection(const Snake *snake, Direction requested) {
    if (requested < DIR_UP || requested >= DIRECTION_COUNT) {
        return snake->m_direction;
    }
    if (snake->m_length > 1 && requested == opposite(snake->m_direction)) {
        return snake->m_direction;
    }
    return requested;
}

/* Translate a coordinate one step in the given direction. */
static Coord moveCoord(Coord pos, Direction dir) {
    switch (dir) {
        case DIR_UP:
            pos.m_row -= 1;
            break;
        case DIR_DOWN:
            pos.m_row += 1;
            break;
        case DIR_LEFT:
            pos.m_col -= 1;
            break;
        case DIR_RIGHT:
        default:
            pos.m_col += 1;
            break;
    }
    return pos;
}

/* The tail is the node whose next link closes the ring back to the head. */
static SnakeNode *findTail(const Snake *snake) {
    SnakeNode *node = snake->m_head;
    while (node->m_next != snake->m_head) {
        node = node->m_next;
    }
    return node;
}

/* The node immediately before the given node in the ring. */
static SnakeNode *findBefore(const Snake *snake, const SnakeNode *node) {
    SnakeNode *cur = snake->m_head;
    while (cur->m_next != node) {
        cur = cur->m_next;
    }
    return cur;
}

/* Check whether a cell lies on or beyond the outer wall ring. */
static bool hitsWall(const Snake *snake, Coord pos) {
    return pos.m_row < BOARD_WALL_THICKNESS ||
           pos.m_row >= snake->m_rows - BOARD_WALL_THICKNESS ||
           pos.m_col < BOARD_WALL_THICKNESS ||
           pos.m_col >= snake->m_cols - BOARD_WALL_THICKNESS;
}

/* ------------------------------------------------------------------------- */
/* Initialization                                                             */
/* ------------------------------------------------------------------------- */

bool snakeInit(Snake *snake, int rows, int cols, size_t length) {
    /* TODO: Make a ring. Start in the middle, facing right.
     * On failure, free any nodes already made and leave an empty snake. */
    if (snake == NULL) {
        return false;
    }
    if (rows < MIN_BOARD_SIZE || cols < MIN_BOARD_SIZE) {
        *snake = (Snake){0};
        return false;
    }
    if (length < 1 || length > (size_t)(cols - 2 * BOARD_WALL_THICKNESS)) {
        *snake = (Snake){0};
        return false;
    }

    snake->m_head = NULL;
    snake->m_length = 0;
    snake->m_direction = DIR_RIGHT;
    snake->m_rows = rows;
    snake->m_cols = cols;

    const int headRow = rows / 2;
    const int headCol = cols / 2 > (int)length ? cols / 2 : (int)length;

    /* Build the ring from head to tail: the head sits at (headRow, headCol);
     * each remaining body node extends one column to the left. */
    SnakeNode *prev = NULL;
    SnakeNode *head = NULL;
    for (size_t i = 0; i < length; i++) {
        SnakeNode *node = malloc(sizeof(SnakeNode));
        if (node == NULL) {
            SnakeNode *cur = head;
            while (cur != NULL) {
                SnakeNode *next = cur->m_next;
                free(cur);
                cur = next;
            }
            *snake = (Snake){0};
            return false;
        }
        node->m_pos.m_row = headRow;
        node->m_pos.m_col = headCol - (int)i;
        node->m_next = NULL;

        if (prev == NULL) {
            head = node;
        } else {
            prev->m_next = node;
        }
        prev = node;
    }

    /* Close the ring: the tail links back to the head. */
    prev->m_next = head;
    snake->m_head = head;
    snake->m_length = length;
    snake->m_direction = DIR_RIGHT;
    return true;
}

/* ------------------------------------------------------------------------- */
/* Queries                                                                    */
/* ------------------------------------------------------------------------- */

Coord snakeNextPosition(const Snake *snake, Direction requested) {
    /* TODO: Return the next head position. Do not change the snake.
     * Ignore a turn straight back when the snake has more than one node. */
    Direction dir = effectiveDirection(snake, requested);
    return moveCoord(snake->m_head->m_pos, dir);
}

bool snakeContains(const Snake *snake, Coord pos) {
    /* TODO: Check every node once. The ring does not end at NULL. */
    if (snake == NULL || snake->m_head == NULL || snake->m_length == 0) {
        return false;
    }
    SnakeNode *cur = snake->m_head;
    for (size_t i = 0; i < snake->m_length; i++) {
        if (cur->m_pos.m_row == pos.m_row && cur->m_pos.m_col == pos.m_col) {
            return true;
        }
        cur = cur->m_next;
    }
    return false;
}

/* ------------------------------------------------------------------------- */
/* Advancement                                                               */
/* ------------------------------------------------------------------------- */

StepResult snakeStep(Snake *snake, Direction requested, bool grow) {
    /* TODO: Check the next position before changing the ring.
     * Reuse the tail on a normal move; add one node when growing.
     * A normal move may enter the old tail's cell. A growing move may not. */
    if (snake == NULL || snake->m_head == NULL) {
        return STEP_NO_MEMORY;
    }

    const Direction dir = effectiveDirection(snake, requested);
    const Coord next = moveCoord(snake->m_head->m_pos, dir);

    /* 1) Wall collision. */
    if (hitsWall(snake, next)) {
        return STEP_WALL;
    }

    SnakeNode *tail = findTail(snake);
    const bool nextIsTail = !grow &&
                            next.m_row == tail->m_pos.m_row &&
                            next.m_col == tail->m_pos.m_col;

    /* 2) Body collision (the vacated tail cell is exempt on a normal move). */
    if (!nextIsTail && snakeContains(snake, next)) {
        return STEP_SELF;
    }

    if (grow) {
        /* Allocate one new head; keep every existing node in place. */
        SnakeNode *newHead = malloc(sizeof(SnakeNode));
        if (newHead == NULL) {
            return STEP_NO_MEMORY;
        }
        newHead->m_pos = next;
        newHead->m_next = snake->m_head;
        snake->m_head = newHead;
        snake->m_length += 1;
    } else {
        /* Reuse the old tail as the new head; no allocation or free. */
        SnakeNode *oldTail = tail;
        SnakeNode *beforeTail = findBefore(snake, oldTail);
        oldTail->m_pos = next;
        oldTail->m_next = snake->m_head;
        beforeTail->m_next = oldTail;
        snake->m_head = oldTail;
    }

    snake->m_direction = dir;
    return STEP_OK;
}

/* ------------------------------------------------------------------------- */
/* Destruction                                                                */
/* ------------------------------------------------------------------------- */

void snakeDestroy(Snake *snake) {
    /* TODO: Free every node, then reset the snake. */
    if (snake == NULL) {
        return;
    }
    if (snake->m_head != NULL) {
        SnakeNode *cur = snake->m_head;
        for (size_t i = 0; i < snake->m_length; i++) {
            SnakeNode *next = cur->m_next;
            free(cur);
            cur = next;
        }
    }
    *snake = (Snake){0};
}

/* ------------------------------------------------------------------------- */
/* Randomness                                                                 */
/* ------------------------------------------------------------------------- */

unsigned int nextRandom(unsigned int seed) {
    /* TODO: Keep one static unsigned int state, initially RANDOM_INITIAL_SEED.
     * A positive seed resets it modulo RANDOM_MODULUS. Then take one
     * recurrence step and return the new state. Zero continues the sequence. */
    static unsigned int state = RANDOM_INITIAL_SEED;

    if (seed != 0) {
        state = seed % RANDOM_MODULUS;
    }
    state = (RANDOM_MULTIPLIER * state + RANDOM_INCREMENT) % RANDOM_MODULUS;
    return state;
}
