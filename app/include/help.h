#ifndef HELP_H
#define HELP_H

// Toggle the help overlay visibility.
void toggle_help(void);

// Return whether the help overlay is visible.
int is_help_visible(void);

// Draw the help overlay in screen space.
void draw_help_overlay(int w, int h);

#endif
