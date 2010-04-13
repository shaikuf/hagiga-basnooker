#ifndef _CALIBRATION_H
#define _CALIBRATION_H

/* this handles computation of the distortion matrices of the camera */
void calibration(int board_w, int board_h, int n_boards, float square_width,
				 float square_height, CvSize resolution, int device_id);

/* this handles generation of perspective wrapping matrix */
void birds_eye(int board_w, int board_h, float board_width, float board_height,
			   CvSize Resolution, int device_id);

/* this lets the user click on balls and save them as templates */
void grab_templates(CvSize resolution, int device_id);

/* mouse callback wrapper for finding a ball around the mouse and saving it
as a template */
void saveTemplateAroundMouse(int event, int x, int y, int flags, void *param);

void learn_borders(CvSize resolution, int device_id);
void borderPointAroundMouse(int event, int x, int y, int flags, void *param);

struct border_data {
	IplImage *img;
	CvSeqWriter *writer;
	CvSize *resolution;
};

#endif