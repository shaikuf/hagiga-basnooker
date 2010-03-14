#ifndef _CALIBRATION_H
#define _CALIBRATION_H

/* this handles computation of the distortion matrices of the camera */
void calibration(int board_w, int board_h, int n_boards, float square_size,
				 CvSize resolution);

/* this handles generation of perspective wrapping matrix */
void birds_eye(int board_w, int board_h, CvSize resolution);

#endif