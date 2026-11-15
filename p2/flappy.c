#include "flappy.h"

#include <stddef.h>

void advancePipes(Game *game, float dt) {
    /* TODO 1: Move every pipe left by PIPE_SPEED * dt. Then visit the slots
     * in array order; if a pipe's right edge is <= 0, call recyclePipe. */
    if (game == NULL || dt < 0.0f) {
        return;
    }

    for (int i = 0; i < PIPE_COUNT; i++) {
        game->m_pipes[i].m_x -= PIPE_SPEED * dt;
    }

    for (int i = 0; i < PIPE_COUNT; i++) {
        if (game->m_pipes[i].m_x + PIPE_WIDTH <= 0.0f) {
            recyclePipe(game, i);
        }
    }
}

bool birdHitsObstacle(const Game *game) {
    /* TODO 2: Check the top/floor boundaries and both rectangles of each
     * pipe pair. Use gameBirdBox and boxesOverlap. Do not change the game. */
    if (game == NULL) {
        return false;
    }

    Box bird = gameBirdBox(game);

    /* Ceiling / floor bounds. Exact contact with the ceiling or floor is safe. */
    if (bird.m_y < 0.0f || bird.m_y + bird.m_height > FLOOR_Y) {
        return true;
    }

    for (int i = 0; i < PIPE_COUNT; i++) {
        const Pipe *pipe = &game->m_pipes[i];

        /* Upper pipe: from y = 0 down to the gap's top edge. */
        Box upper = {
            pipe->m_x, 0.0f,
            PIPE_WIDTH, pipe->m_gap_y
        };
        /* Lower pipe: from the gap's bottom edge to the floor. */
        Box lower = {
            pipe->m_x, pipe->m_gap_y + PIPE_GAP_HEIGHT,
            PIPE_WIDTH, FLOOR_Y - pipe->m_gap_y - PIPE_GAP_HEIGHT
        };

        if (boxesOverlap(bird, upper) || boxesOverlap(bird, lower)) {
            return true;
        }
    }
    return false;
}

int scorePassedPipes(Game *game) {
    /* TODO 3: A pipe scores once when its right edge is strictly left of
     * BIRD_X. Mark it scored, add to m_score, and return this call's count. */
    if (game == NULL) {
        return 0;
    }

    int newPoints = 0;
    for (int i = 0; i < PIPE_COUNT; i++) {
        Pipe *pipe = &game->m_pipes[i];
        if (!pipe->m_scored && pipe->m_x + PIPE_WIDTH < BIRD_X) {
            pipe->m_scored = true;
            game->m_score += 1;
            newPoints += 1;
        }
    }
    return newPoints;
}

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