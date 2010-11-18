#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

// Numero de quadros ate a camera "calibrar"
#define CATCH 60
#define WINSIZEX 300
#define WINSIZEY 210
#define MOV_LIMIT 5
#define PIP_PERCENT (0.3)
//#define FROMFILE


// A Simple Camera Capture Framework 
int main(int argc, char **argv) {
    unsigned long int cont=0;
	struct timeval tv_a, tv_d;
	struct timezone tz;
    CvMoments moments;
    IplImage *frame,
             *fundo,
             *frame_dst,
             *ts,
             *foto,
             *fotoTMP, 
             *pip_frame, 
             *framec, 
             *fotoWIN;
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

    foto_size = cvGetSize(foto);
    ROI = cvGetSize(cvQueryFrame( capture ));

    frame = cvCreateImage(ROI, IPL_DEPTH_8U, 1);
    fundo = cvCreateImage(ROI, IPL_DEPTH_8U, 1);
    frame_dst = cvCreateImage(ROI, IPL_DEPTH_8U, 1);
    ts = cvCreateImage(ROI, IPL_DEPTH_8U, 1);
    pip_frame = cvCreateImage(cvSize(foto_size.width * PIP_PERCENT, foto_size.height * PIP_PERCENT), IPL_DEPTH_8U, 3);
    framec = cvCreateImage(ROI, IPL_DEPTH_8U, 3);
    fotoWIN = cvCreateImage(cvSize(WINSIZEX * 2 , WINSIZEY * 2), IPL_DEPTH_8U, 3);

    cvNamedWindow( "FOTO", CV_WINDOW_AUTOSIZE );
    cvNamedWindow( "WINDOW", CV_WINDOW_AUTOSIZE );
    cvMoveWindow("WINDOW", 700, 0);


//  rLargura = (int) foto_size.width/ROI.width ;
//  rAltura = (int) foto_size.height/ROI.height ;
    razao.x = (double) foto_size.width/ROI.width ;
    razao.y = (double) foto_size.height/ROI.height ;

    margemOLD.x = - MOV_LIMIT - 1;
    margemOLD.y = - MOV_LIMIT - 1;

    while( 1 ) {
		gettimeofday(&tv_a, &tz);
        framec = cvQueryFrame( capture );
        cvCvtColor(framec, frame, CV_RGB2GRAY);

        if( !frame ) {
            fprintf( stderr, "ERROR: frame is null...\n" );
            getchar();
            break;
        }
        if ((cont == CATCH)) {
            cvCopy(frame, fundo, NULL);
            cvSmooth(fundo, fundo, CV_GAUSSIAN, 3, 0, 0, 0);
            printf("Peguei o fundo: [%p]\n", fundo);
        }
        else if ( (cont > CATCH ) && (fundo != frame) ) {
            cvSmooth(frame, frame, CV_GAUSSIAN, 3, 0, 0, 0);
            cvAbsDiff(frame, fundo, frame_dst);

#ifdef FROMFILE
            cvThreshold(frame_dst, ts, 60.0, 255.0, CV_THRESH_BINARY);
#else
            cvThreshold(frame_dst, ts, 30.0, 255.0, CV_THRESH_BINARY);
#endif

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


            cvRectangle(framec, cvPoint(bigr.x, bigr.y), cvPoint(bigr.x + bigr.width, bigr.y + bigr.height), CV_RGB(0,0,255), 2, 8, 0);
            cvRectangle(ts, cvPoint(bigr.x, bigr.y), cvPoint(bigr.x + bigr.width, bigr.y + bigr.height), CV_RGB(255,255,255), CV_FILLED, 8, 0);

            cabeca.x = bigr.x + bigr.width / 2 ;
            cabeca.y = bigr.y + (bigr.height / 8) * 1.5;

            cvCircle(framec, cvPoint(cabeca.x , cabeca.y), 25, CV_RGB(255, 0, 0), 2, 8, 0);

            cvResize(framec, pip_frame, CV_INTER_AREA);
    
                    
			cvCopy(foto, fotoTMP, NULL);
			cvRectangle(fotoTMP, cvPoint( (int)cabeca.x * razao.x - WINSIZEX, (int)cabeca.y * razao.y - WINSIZEY), cvPoint( (int)cabeca.x * razao.x + WINSIZEX, (int)cabeca.y * razao.y + WINSIZEY), CV_RGB(255, 0, 0), 2, 8, 0);

			if ( cabeca.x * razao.x  < WINSIZEX )
	    		margemTL.x = 0;
			else if ( (cabeca.x * razao.x + WINSIZEX) > (foto_size.width - 1) )
				margemTL.x = foto_size.width - 1 - WINSIZEX * 2;
			else
				margemTL.x = (int) cabeca.x * razao.x  - WINSIZEX;

			if ( cabeca.y * razao.y  < WINSIZEY)
				margemTL.y = 0;
			else if ( (cabeca.y * razao.y + WINSIZEY) > (foto_size.height - 1) )
				margemTL.y = foto_size.height - 1 - WINSIZEY * 2;
			else
				margemTL.y = (int) cabeca.y * razao.y  - WINSIZEY;


			if ( abs(margemTL.x - margemOLD.x) > MOV_LIMIT  || abs(margemTL.y - margemOLD.y) > MOV_LIMIT ) {
			
				cvSetImageROI(foto, cvRect( margemOLD.x + (margemTL.x - margemOLD.x), margemOLD.y + (margemTL.y - margemOLD.y), WINSIZEX * 2 , WINSIZEY * 2));
				margemOLD.x = margemTL.x;
			}

			cvCopy(foto, fotoWIN, NULL);
			cvResetImageROI(foto);
			cvShowImage("WINDOW", fotoWIN);
		}
    	else 
			cvCopy(foto, fotoTMP, NULL);

        //  Criando PIP
        cvSetImageROI(fotoTMP, cvRect( 0, 0, foto_size.width * PIP_PERCENT , foto_size.height * PIP_PERCENT));
        cvCopy(pip_frame, fotoTMP, NULL);
        cvResetImageROI(fotoTMP);
//      cvShowImage("CAM", framec);
        cvShowImage("FOTO", fotoTMP);

        // Do not release the frame!      
        //If ESC key pressed, Key=0x10001B under OpenCV 0.9.7(linux version),     
        //remove higher bits using AND operator
        if( (cvWaitKey(10) & 255) == 27 )
            break;

		struct timeval tv;
        cont++;
		char buffer[30];
		gettimeofday(&tv_d, &tz);
		tv.tv_sec = tv_d.tv_sec - tv_a.tv_sec;
		tv.tv_usec = tv_d.tv_usec - tv_a.tv_usec;
		strftime(buffer, 30, "%M:%S", localtime(&(tv.tv_sec)));
		printf("Tempo do quadro: %s.%ld\n", buffer, tv.tv_usec);
    }
    // Release the capture device housekeeping   
    cvReleaseCapture( &capture );   
    return 0; 
}      
