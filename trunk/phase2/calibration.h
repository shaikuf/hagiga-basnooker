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
void saveTemplateAroundMouse(int event, int x, int y, int flags, void *param);
/* grab a customized template */
IplImage *overlay_template(IplImage *src, CvPoint center);
/* create a customized overlay */
void setOverlay(IplImage *overlay, int d_width, int d_height, int gradient);

/* this lets the user mark the edges of the projection area */
void learn_edges(bool calibrate, CvSize resolution, int device_id);
void edgePointAroundMouse(int event, int x, int y, int flags, void *param);

/* this lets the user watch the camera image fixed using the matrices */
void watch(CvSize resolution, bool with_birds_eye, int device_id);

/* this fixes the mouse coordinates according to the image resolution */
void fixCoordinates(int &x, int &y, CvSize resolution);

/* this creates the birds-eye matrix from the edges of the projection image */
void calibrationFromEdges(CvSeq *edges, CvSize resolution, int device_id);

/* this is used to pass more parameters the the mouse callback functions */
struct seq_data {
	IplImage *img;
	CvSeqWriter *writer;
	CvSize *resolution;
};

#endif