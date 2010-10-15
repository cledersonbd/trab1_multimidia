#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>
#include <stdio.h>

// Numero de quadros ate a camera "calibrar"
#define CATCH 60
#define WINSIZE 150
#define MOV_LIMIT 3
//#define FROMFILE


// A Simple Camera Capture Framework 
int main(int argc, char **argv) {
	unsigned long int cont=0;
	IplImage* frame;
	IplImage* fundo ;
	IplImage* frame_dst;
	IplImage* ts;
	CvMoments* moments;
	IplImage *foto, *fotoTMP, *ts_cor, *framec, *fotoWIN;
	CvSize ROI, foto_size;
	CvPoint cabeca, margemTL, margemOLD;
	CvPoint2D64f razao;
	CvRect retangulo, bigr;
	CvSeq *seq;
	CvMemStorage *storage;

#ifdef FROMFILE
	CvCapture* capture = cvCaptureFromFile( argv[2] );
#else
	CvCapture* capture = cvCaptureFromCAM( CV_CAP_ANY );
#endif

	if( !capture ) {
				fprintf( stderr, "ERROR: capture is NULL \n" );
		getchar();
		return -1;
	}
	
	foto = cvLoadImage(argv[1], -1);
	fotoTMP = cvLoadImage(argv[1], -1);
//	fotoWIN = cvLoadImage(argv[1], -1);

	foto_size = cvGetSize(foto);
	ROI = cvGetSize(cvQueryFrame( capture ));

//	cvSetImageROI(foto, cvRect(200, 200, ROI.width, ROI.height));

	frame = cvCreateImage(ROI, IPL_DEPTH_8U, 1);
	fundo = cvCreateImage(ROI, IPL_DEPTH_8U, 1);
	frame_dst = cvCreateImage(ROI, IPL_DEPTH_8U, 1);
	ts = cvCreateImage(ROI, IPL_DEPTH_8U, 1);
	ts_cor = cvCreateImage(ROI, IPL_DEPTH_8U, 3);
	framec = cvCreateImage(ROI, IPL_DEPTH_8U, 3);
	fotoWIN = cvCreateImage(cvSize(WINSIZE * 2 , WINSIZE * 2), IPL_DEPTH_8U, 3);

//	cvNamedWindow( "Frame", CV_WINDOW_AUTOSIZE );
//	cvNamedWindow( "Fundo", CV_WINDOW_AUTOSIZE );
//	cvNamedWindow( "Sub", CV_WINDOW_AUTOSIZE );
//	cvNamedWindow( "TS", CV_WINDOW_AUTOSIZE );
	cvNamedWindow( "CAM", CV_WINDOW_AUTOSIZE );
	cvNamedWindow( "FOTO", CV_WINDOW_AUTOSIZE );
	cvNamedWindow( "WINDOW", CV_WINDOW_AUTOSIZE );

//	cvShowImage("FOTO", foto);

//	rLargura = (int) foto_size.width/ROI.width ;
//	rAltura = (int) foto_size.height/ROI.height ;
	razao.x = (double) foto_size.width/ROI.width ;
	razao.y = (double) foto_size.height/ROI.height ;

	margemOLD.x = - MOV_LIMIT - 1;
	margemOLD.y = - MOV_LIMIT - 1;

	while( 1 ) {
//		usleep(100000);
		framec = cvQueryFrame( capture );
		cvCvtColor(framec, frame, CV_RGB2GRAY);

		if( !frame ) {
			fprintf( stderr, "ERROR: frame is null...\n" );
			getchar();
			break;
		}
		if ((cont == CATCH)) {
			cvCopy(frame, fundo, NULL);
			printf("Peguei o fundo: [%p]\n", fundo);
		}
		else if ( (cont > CATCH ) && (fundo != frame) ) {
//			cvShowImage("Fundo", fundo);		
//			cvShowImage("Frame", frame);

			cvSub(fundo, frame, frame_dst, NULL);

			cvSmooth(frame_dst, frame_dst, CV_GAUSSIAN, 3, 0, 0, 0);
//			cvShowImage("Sub", frame_dst);

			//	Aumenta o contraste da diferenca
//			cvMul(frame_dst,  frame_dst, frame_dst, 1);
//			cvShowImage("Sub", frame_dst);
#ifdef FROMFILE
			cvThreshold(frame_dst, ts, 45.0, 255.0, CV_THRESH_BINARY);
#else
			cvThreshold(frame_dst, ts, 10.0, 255.0, CV_THRESH_BINARY);
#endif
//			cvShowImage("TS", ts);

			storage = cvCreateMemStorage(0);
			cvClearMemStorage(storage);
			cvFindContours(ts, storage, &seq, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE, cvPoint(0,0));

			bigr.width = 0;
			bigr.height = 0;
			for(; seq; seq = seq->h_next) {
				retangulo = cvBoundingRect(seq, 0);
				if ( (retangulo.width * retangulo.height) > (bigr.width * bigr.height) ) {
					bigr.x = retangulo.x;
					bigr.y = retangulo.y;
					bigr.width = retangulo.width;
					bigr.height = retangulo.height;
				}
			}


			cvRectangle(framec, cvPoint(retangulo.x, retangulo.y), cvPoint(retangulo.x + retangulo.width, retangulo.y + retangulo.height), CV_RGB(0,0,255), 2, 8, 0);

			cvMoments(ts, moments, 1);

//			printf("m00: %f | m10: %f | m01: %f | %fx%f\n", moments->m00, moments->m10, moments->m01, moments->m10 / moments->m00, moments->m01 / moments->m00);

			//	Calcula centro de massa
			cabeca.x = moments->m10 / moments->m00 ;
//			cabeca.y = moments->m01 / moments->m00 / 2 / 2 ;
			cabeca.y = moments->m01 / moments->m00 ;

			cvCvtColor(ts, ts_cor, CV_GRAY2RGB);

			cvCircle(framec, cvPoint(cabeca.x , cabeca.y), 25, CV_RGB(255, 0, 0), 2, 8, 0);
//			cvShowImage("Resultado", frame_dst);
	
			if (moments->m00 > 0.0) {
					
//				cvSetImageROI(foto, cvRect((int) (moments->m10 / moments->m00), (int) (moments->m01 / moments->m00 / 2 / 2), ROI.width, ROI.height));
//				cvSetImageROI(foto, cvRect((int) (moments->m10 / moments->m00), (int) (moments->m01 / moments->m00 / 2 / 2), ROI.width, ROI.height));

				cvCopy(foto, fotoTMP, NULL);
				cvRectangle(fotoTMP, cvPoint( (int)cabeca.x * razao.x - WINSIZE, (int)cabeca.y * razao.y - WINSIZE), cvPoint( (int)cabeca.x * razao.x + WINSIZE, (int)cabeca.y * razao.y + WINSIZE), CV_RGB(255, 0, 0), 2, 8, 0);
//				cvSetImageROI(foto, cvRect(cabeca.x * razao.x - WINSIZE, cabeca.y * razao.y - WINSIZE, WINSIZE * 2 , WINSIZE * 2));

				if ( cabeca.x * razao.x  < WINSIZE )
					margemTL.x = 0;
				else if ( (cabeca.x * razao.x + WINSIZE) > (foto_size.width - 1) )
					margemTL.x = foto_size.width - 1 - WINSIZE * 2;
				else
					margemTL.x = (int) cabeca.x * razao.x  - WINSIZE;

				if ( cabeca.y * razao.y  < WINSIZE)
					margemTL.y = 0;
				else if ( (cabeca.y * razao.y + WINSIZE) > (foto_size.height - 1) )
					margemTL.y = foto_size.height - 1 - WINSIZE * 2;
				else
					margemTL.y = (int) cabeca.y * razao.y  - WINSIZE;

				printf("margemOLD: %dx%d | ", margemOLD.x, margemOLD.y);

				if ( (margemTL.x > (margemOLD.x + MOV_LIMIT) ) || (margemTL.y > (margemOLD.y + MOV_LIMIT)) ) {
					cvSetImageROI(foto, cvRect( margemTL.x, margemTL.y, WINSIZE * 2 , WINSIZE * 2));
					printf("Se mexeu | ");
					margemOLD.x = margemTL.x;
					margemOLD.y = margemTL.y;
				}
				else {
					cvSetImageROI(foto, cvRect( margemOLD.x, margemOLD.y, WINSIZE * 2 , WINSIZE * 2));
					printf("Nao mexeu | ");
				}

//				margemOLD.x = margemTL.x;
//				margemOLD.y = margemTL.y;

				printf("CAM %dx%d | TL_CPY: %dx%d | CabecaFOTO: x:%d y:%d | foto %dx%d | fotoWIN %dx%d\n", ROI.width, ROI.height, margemTL.x, margemTL.y, (int) (cabeca.x * razao.x) , (int) (cabeca.y * razao.y), foto->roi->width, foto->roi->height, fotoWIN->width, fotoWIN->height);

				cvCopy(foto, fotoWIN, NULL);
				cvResetImageROI(foto);

//				cvSetImageROI(fotoTMP, cvRect(0, 0, ROI.width, ROI.height));
//				cvCopy(framec, fotoTMP, NULL);
//				cvResetImageROI(fotoTMP);
				cvShowImage("WINDOW", fotoWIN);
				cvShowImage("FOTO", fotoTMP);

			}
			else {

				cvCopy(foto, fotoTMP, NULL);
//				cvSetImageROI(fotoTMP, cvRect(0, 0, ROI.width, ROI.height));
//				cvCopy(ts_cor, fotoTMP, NULL);
//				cvResetImageROI(fotoTMP);
				cvShowImage("FOTO", fotoTMP);


			}

		}
		cvShowImage("CAM", framec);

//		cvShowImage( "Work", ts);
//		cvShowImage("Fundo", fundo);		
//		cvShowImage("Frame", frame);

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
