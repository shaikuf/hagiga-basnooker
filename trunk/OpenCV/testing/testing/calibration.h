#ifndef _CALIBRATION_H
#define _CALIBRATION_H

void calibration(int board_w, int board_h, int n_boards, float square_size,
				 CvSize resolution);

int birds_eye(int argc, char *argv[]);

void brakha_main(int board_w /*n*/, int board_h /*m*/, int N, CvSize image_sz);

#endif