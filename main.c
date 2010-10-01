#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>
#include <stdio.h>

// Numero de quadros ate a camera "calibrar"
#define CATCH 30

// A Simple Camera Capture Framework 
int main() {
	unsigned long int cont=0;
	IplImage* frame;
	IplImage* fundo ;
	IplImage* frame_dst;
	IplImage* ts;

	CvCapture* capture = cvCaptureFromCAM( CV_CAP_ANY );

	if( !capture ) {
		fprintf( stderr, "ERROR: capture is NULL \n" );
		getchar();
		return -1;
	}
	
	// Brilho da camera
	cvSetCaptureProperty(capture, CV_CAP_PROP_BRIGHTNESS, 150);

	frame = cvQueryFrame( capture );


	frame = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
	fundo = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
	frame_dst = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
	ts = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);

	cvNamedWindow( "Normal", CV_WINDOW_AUTOSIZE );
	cvNamedWindow( "Work", CV_WINDOW_AUTOSIZE );

	while( 1 ) {
//		cvSplit(cvQueryFrame( capture ), frame, NULL, NULL, NULL);
		cvCvtColor(cvQueryFrame( capture ), frame, CV_RGB2GRAY);
		cvSmooth(frame, frame, CV_GAUSSIAN, 7, 7, 0, 0);

		if (cont == CATCH) {
//			cvSplit(cvQueryFrame( capture ), fundo, NULL, NULL, NULL);
			cvCvtColor(cvQueryFrame( capture ), fundo, CV_RGB2GRAY);
			cvSmooth(fundo, fundo, CV_GAUSSIAN, 7, 7, 0, 0);
			printf("Peguei o fundo: [%p]\n", fundo);
		}

		if( !frame ) {
			fprintf( stderr, "ERROR: frame is null...\n" );
			getchar();
			break;
		}

		if ( (cont > CATCH ) &&fundo && (fundo != frame) ) {
//			cvSplit(cvQueryFrame( capture ), frame_dst, NULL, NULL, NULL);
			cvCvtColor(cvQueryFrame( capture ), frame_dst, CV_RGB2GRAY);

			// subtracao do fundo			 
			cvSub(fundo, frame, frame_dst, NULL);
			// suavizando a bagaca
			cvSmooth(frame_dst, ts, CV_GAUSSIAN, 7, 7, 0, 0);
			// aplicando limiar
			cvThreshold(ts, ts, 15.0, 255.0, CV_THRESH_BINARY);
			// erodindo pra aumentar a eficiencia do blob
			cvErode(ts, ts, NULL, 2);
//			cvThreshold(frame_dst, ts, 5.0, 255.0, CV_THRESH_TOZERO);

			cvShowImage( "Work", ts);
		}

		cvShowImage("Normal", frame);
		// Do not release the frame!      
		//If ESC key pressed, Key=0x10001B under OpenCV 0.9.7(linux version),     
		//remove higher bits using AND operator
		if( (cvWaitKey(10) & 255) == 27 )
			break;

		cont++;
	}
	// Release the capture device housekeeping   
	cvReleaseCapture( &capture );   
	cvDestroyWindow( "mywindow" );
	return 0; 
}      
