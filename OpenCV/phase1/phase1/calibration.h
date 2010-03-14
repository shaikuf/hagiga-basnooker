#ifndef _CALIBRATION_H
#define _CALIBRATION_H

/* this handles computation of the distortion matrices of the camera */
void calibration(int board_w, int board_h, int n_boards, float square_size,
				 CvSize resolution);

/* this handles generation of perspective wrapping matrix */
void birds_eye(int board_w, int board_h, CvSize resolution);

/* this lets the user click on balls and save them as templates */
void grab_templates(CvSize resolution);

/* mouse callback wrapper for finding a ball around the mouse and saving it
as a template */
void saveTemplateAroundMouse(int event, int x, int y, int flags, void *param);

#endif