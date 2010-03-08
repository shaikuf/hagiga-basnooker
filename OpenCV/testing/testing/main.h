#ifndef _MAIN_H
#define _MAIN_H

/* Shows an image and forwared clicks on it to the given function until ESC is
met */
void genericMouseWrapper(IplImage *img, void (*func)(int, int, int, int, void *));

/* Mouse callback wrapper that prints the color of the clicked point. param
is the img to work on */
void findColorAroundMouse(int event, int x, int y, int flags, void *param);

/* Mouse callback wrapper that prints the position of the clicked point.
param is the img to work on */
void findPosAroundMouse(int event, int x, int y, int flags, void *param);

/* Mouse callback wrapper for findBallAround. param is the img to work on */
void findBallAroundMouse(int event, int x, int y, int flags, void *param);

/* The main loop of the program */
void gameLoop();

#endif _MAIN_H