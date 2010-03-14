#ifndef _CALIBRATION_H
#define _CALIBRATION_H

typedef struct ball {
	CvPoint2D32f pos;
	float radius;
} BALL;

void calibration(int board_w, int board_h, int n_boards, float square_size,
				 CvSize resolution);

void birds_eye(int board_w, int board_h, CvSize resolution);

#endif