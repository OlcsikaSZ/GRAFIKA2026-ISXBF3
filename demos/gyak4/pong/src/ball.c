#include "ball.h"
#include "game.h"  // Új include a játék pontjainak kezeléséhez
#include <GL/gl.h>
#include <math.h>

/**
 * Initialize the ball at position (x, y) with increased speed.
 */
void init_ball(Ball* ball, float x, float y)
{
    ball->x = x;
    ball->y = y;
    ball->radius = 50;  // Labda mérete
    ball->speed_x = 200; // Gyorsabb vízszintes sebesség
    ball->speed_y = 150; // Gyorsabb függőleges sebesség
    ball->rotation_angle = 0; // Labda kezdeti forgási szöge
}

/**
 * Update the ball's position, handle wall collisions, and update its rotation.
 * This function assumes there are two paddles: left and right.
 */
void update_ball(Ball* ball, double time, float paddle_left_x, float paddle_left_y, float paddle_right_x, float paddle_right_y, float paddle_width, float paddle_height, int* left_player_score, int* right_player_score)
{
    ball->x += ball->speed_x * time;
    ball->y += ball->speed_y * time;

    // Ball rotation update
    ball->rotation_angle += 200 * time;
    if (ball->rotation_angle >= 360) ball->rotation_angle -= 360;

    // Ball collision with top and bottom of the screen
    if (ball->y - ball->radius < 0 || ball->y + ball->radius > 600) {
        ball->speed_y *= -1;
    }

    // Ball collision with paddles on the left and right
    if (ball->x - ball->radius < paddle_left_x + paddle_width &&
        ball->y > paddle_left_y &&
        ball->y < paddle_left_y + paddle_height) {
        ball->speed_x *= -1;  // Reverse direction if it hits the left paddle
    } else if (ball->x - ball->radius < 0) {
        (*right_player_score)++;
        init_ball(ball, 400, 300); // Reset ball to the center
    }

    if (ball->x + ball->radius > paddle_right_x &&
        ball->y > paddle_right_y &&
        ball->y < paddle_right_y + paddle_height) {
        ball->speed_x *= -1;  // Reverse direction if it hits the right paddle
    } else if (ball->x + ball->radius > 800) {
        (*left_player_score)++;
        init_ball(ball, 400, 300); // Reset ball to the center
    }
}

/**
 * Render the ball at its current position, maintaining its size and color, and applying rotation.
 */
void render_ball(Ball* ball)
{
    double angle;
    double x;
    double y;

    glPushMatrix();
    glTranslatef(ball->x, ball->y, 0.0);
    glRotatef(ball->rotation_angle, 0, 0, 1);  // Rotate the ball
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(1.0, 0.9, 0.8);
    glVertex2f(0, 0);
    angle = 0;
    while (angle < 2.0 * M_PI + 1) {
        x = cos(angle) * ball->radius;
        y = sin(angle) * ball->radius;
        glVertex2f(x, y);
        angle += 0.8;
    }
    glEnd();
    glPopMatrix();
}

/**
 * Move the ball to a new position based on mouse click.
 */
void set_ball_position(Ball* ball, float new_x, float new_y)
{
    ball->x = new_x;
    ball->y = new_y;
}

/**
 * Change the ball's size based on key input, within defined limits.
 */
void resize_ball(Ball* ball, float delta_radius)
{
    float new_radius = ball->radius + delta_radius;
    if (new_radius >= 20 && new_radius <= 100) {
        ball->radius = new_radius;
    }
}