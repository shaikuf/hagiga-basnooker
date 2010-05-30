#ifndef _MAIN_H
#define _MAIN_H

/*	The main loop of the program.
	Gets:
		(CvSize)resolution	the resolution of the camera to use
		(int)device_id	the id of the camera to use	*/
void gameLoop(CvSize resolution, int device_id);

#endif _MAIN_H