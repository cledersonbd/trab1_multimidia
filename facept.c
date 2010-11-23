/* Demo of modified Lucas-Kanade optical flow algorithm.
   See the printf below */

#include "cv.h"
#include "highgui.h"
#include <stdio.h>
#include <ctype.h>

#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>

#define FACE_DETECT_STEP 3
#define WINSIZEX 400
#define WINSIZEY 210
#define MOV_LIMIT 5

#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

const char* cascade_name = "haarcascade_frontalface_alt.xml";

struct face_s {
    IplImage *img;
    CvPoint pt1, pt2;
    int area;
    int detected;
    int num_frames;
};

CvMemStorage* storage = 0;
CvHaarClassifierCascade* cascade = 0;


IplImage *image = 0, *grey = 0, *prev_grey = 0, *pyramid = 0, *prev_pyramid = 0, *swap_temp;

pthread_mutex_t fakeMutex = PTHREAD_MUTEX_INITIALIZER;
int win_size = 10;
const int MAX_COUNT = 500;
CvPoint2D32f* points[2] = {0,0}, *swap_points;
char* status = 0;
int count = 0;
int need_to_init = 0;
int night_mode = 0;
int flags = 0;
int add_remove_pt = 0;
CvPoint pt[30];

void * detect_face (void *param) {

  struct face_s *face;
  int i;
  int scale = 1;
  IplImage *img;

  face = (struct face_s *) param;

//  for(;;) {

    //if ( face->num_frames % FACE_DETECT_STEP != 0 ) continue;

    //if(face->img == NULL) { continue;}
    if(face->img == NULL) { return;}

    img = cvCreateImage( cvGetSize(face->img), 8, 3 );
    cvCopy( face->img, img, 0 );

    CvSeq* faces = cvHaarDetectObjects( img, cascade, storage,
                                 1.1, 2, CV_HAAR_DO_CANNY_PRUNING, cvSize(40, 40) );
    if(faces->total > 0) {
        face->detected =1;


      CvRect* r = (CvRect*)cvGetSeqElem( faces, 0 );
      //Find the dimensions of the face,and scale it if necessary
      face->pt1.x = r->x*scale;
      face->pt2.x = (r->x+r->width)*scale;
      face->pt1.y = r->y*scale;
      face->pt2.y = (r->y+r->height)*scale;
      face->area = (face->pt2.x - face->pt1.x) * (face->pt2.y - face->pt1.y);

     }else { face->detected=0;}

      // Draw the rectangle in the input image
      //cvRectangle( img, face->pt1, face->pt2, CV_RGB(255,0,0), 3, 8, 0 );

      

    if(count < 12) {
        add_remove_pt=0;
        if(count + add_remove_pt < 12)
      pt[add_remove_pt++] = cvPoint( (face->pt1.x+face->pt2.x)/2, (face->pt1.y+face->pt2.y)/2);
        if(count + add_remove_pt < 12)
      pt[add_remove_pt++] = cvPoint( pt[0].x+4 , pt[0].y);
        if(count + add_remove_pt < 12)
      pt[add_remove_pt++] = cvPoint( pt[0].x-4 , pt[0].y);
        if(count + add_remove_pt < 12)
      pt[add_remove_pt++] = cvPoint( pt[0].x , pt[0].y+4);
        if(count + add_remove_pt < 12)
      pt[add_remove_pt++] = cvPoint( pt[0].x , pt[0].y-4);
        if(count + add_remove_pt < 12)
      pt[add_remove_pt++] = cvPoint( pt[0].x+8 , pt[0].y);
        if(count + add_remove_pt < 12)
      pt[add_remove_pt++] = cvPoint( pt[0].x-8 , pt[0].y);
        if(count + add_remove_pt < 12)
      pt[add_remove_pt++] = cvPoint( pt[0].x , pt[0].y+8);
        if(count + add_remove_pt < 12)
      pt[add_remove_pt++] = cvPoint( pt[0].x , pt[0].y-8);
        if(count + add_remove_pt < 12)
      pt[add_remove_pt++] = cvPoint( pt[0].x+4 , pt[0].y+4);
        if(count + add_remove_pt < 12)
      pt[add_remove_pt++] = cvPoint( pt[0].x-4 , pt[0].y-4);
        if(count + add_remove_pt < 12)
      pt[add_remove_pt++] = cvPoint( pt[0].x+4 , pt[0].y+4);
        if(count + add_remove_pt < 12)
      pt[add_remove_pt++] = cvPoint( pt[0].x-4 , pt[0].y-4);
    //points[1][count++] = cvPointTo32f(pt);
      //      cvFindCornerSubPix( grey, points[1] + count - 1, 1,
      //          cvSize(win_size,win_size), cvSize(-1,-1),
       //         cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03));
    }

//  }

} //end detect_face

void on_mouse( int event, int x, int y, int flags, void* param )
{
    if( !image )
        return;

    if( image->origin )
        y = image->height - y;

    if( event == CV_EVENT_LBUTTONDOWN )
    {
        pt[0] = cvPoint(x,y);
        add_remove_pt = 1;
    }
}


int main( int argc, char** argv )
{

    CvCapture* capture = 0;
    pthread_attr_t attr;
    pthread_t thread;
    IplImage *foto,*wind;
    int s;
    CvPoint2D64f razao;
    CvPoint cabeca, margemTL, margemOLD;
    struct face_s face = { .detected = 0, .img = NULL, .num_frames = 0 };
    CvSize ROI, foto_size,cam;

    foto = cvLoadImage(argv[1], -1);
    foto_size = cvGetSize(foto);

    razao.x = (double) foto_size.width/ROI.width ;
    razao.y = (double) foto_size.height/ROI.height ;

    margemOLD.x = - MOV_LIMIT - 1;
    margemOLD.y = - MOV_LIMIT - 1;

    cvNamedWindow( "foto", 1 );



    if( argc == 2 )
        capture = cvCaptureFromCAM( 0 );
    else if( argc == 3 )
        capture = cvCaptureFromAVI( argv[2] );


    if( !capture )
    {
        fprintf(stderr,"Could not initialize capturing...\n");
        return -1;
    }

    storage = cvCreateMemStorage(0);
    cascade = (CvHaarClassifierCascade*)cvLoad( cascade_name, 0, 0, 0 );
    // Check whether the cascade has loaded successfully. Else report and error and quit
    cvClearMemStorage( storage );
    if( !cascade ) {
            fprintf( stderr, "ERROR: Could not load classifier cascade\n" );
            return;
    }


    /* print a welcome message, and the OpenCV version */
    printf ("Welcome to lkdemo, using OpenCV version %s (%d.%d.%d)\n",
	    CV_VERSION,
	    CV_MAJOR_VERSION, CV_MINOR_VERSION, CV_SUBMINOR_VERSION);

    printf( "Hot keys: \n"
            "\tESC - quit the program\n"
            "\tr - auto-initialize tracking\n"
            "\tc - delete all the points\n"
            "\tn - switch the \"night\" mode on/off\n"
            "To add/remove a feature point click it\n" );

    cvNamedWindow( "LkDemo", 1 );
    cvSetMouseCallback( "LkDemo", on_mouse, 0 );

    s = pthread_attr_init(&attr);
    if (s != 0)
       handle_error_en(s, "pthread_attr_init");

//    s = pthread_create(&thread, &attr,
//                       &detect_face, &face);
//    if (s != 0)
//        handle_error_en(s, "pthread_create");

    foto = cvLoadImage(argv[1], -1);

    for(;;)
    {
        IplImage* frame = 0;
        int i, k, c;

        frame = cvQueryFrame( capture );
    cam = cvGetSize(frame);
        if( !frame )
            break;

        if( !image )
        {
            /* allocate all the buffers */
            image = cvCreateImage( cvGetSize(frame), 8, 3 );
            image->origin = frame->origin;
            grey = cvCreateImage( cvGetSize(frame), 8, 1 );
            prev_grey = cvCreateImage( cvGetSize(frame), 8, 1 );
            pyramid = cvCreateImage( cvGetSize(frame), 8, 1 );
            prev_pyramid = cvCreateImage( cvGetSize(frame), 8, 1 );
            points[0] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(points[0][0]));
            points[1] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(points[0][0]));
            status = (char*)cvAlloc(MAX_COUNT);
            flags = 0;
        }

        cvCopy( frame, image, 0 );
        cvCvtColor( image, grey, CV_BGR2GRAY );
        face.img = image;

if(count < 10 && face.num_frames % 5 == 0)
detect_face(&face);

        if( need_to_init )
        {

            /* automatic initialization */
            IplImage* eig = cvCreateImage( cvGetSize(grey), 32, 1 );
            IplImage* temp = cvCreateImage( cvGetSize(grey), 32, 1 );
            double quality = 0.01;
            double min_distance = 5;

            count = MAX_COUNT;
            cvGoodFeaturesToTrack( grey, eig, temp, points[1], &count,
                                   quality, min_distance, 0, 3, 0, 0.04 );
            cvFindCornerSubPix( grey, points[1], count,
                cvSize(win_size,win_size), cvSize(-1,-1),
                cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03));
            cvReleaseImage( &eig );
            cvReleaseImage( &temp );

            add_remove_pt = 0;
        }
        else if( count > 0 )
        {

            int midx=0,midy=0;
            for( i = k = 0; i < count; i++ ) {
                midx += points[1][i].x;
                midy += points[1][i].y;
            }
            midx /= count;
            midy /= count;
            cvCircle( image, cvPoint(midx,midy), 9, CV_RGB(0,0,255), -1, 8,0);

            cabeca.x = midx;
            cabeca.y = midy;


//printf("%d - %d - %d\n", (float) foto_size.width/100*( (float) midx/cam.width*100 ), midx, cam.width );
            wind = cvCreateImage(foto_size, IPL_DEPTH_8U, 3);
            cvCopy(foto, wind, NULL);
            //cvSetImageROI(wind, cvRect( midx/WINSIZEX*razao.x, 0, WINSIZEX , WINSIZEY ));
            cvShowImage( "foto", wind );

            int i,k;
            for( i = k = 0; i < count; i++ ) {

                if( (int) points[1][i].x-midx > 40 || (int)points[1][i].x-midx < -40)
                    continue;
                if( (int) points[1][i].y-midy > 40 || (int)points[1][i].y-midy < -40)
                    continue;
                points[1][k++] = points[1][i];
            }
            count = k;

            cvCalcOpticalFlowPyrLK( prev_grey, grey, prev_pyramid, pyramid,
                points[0], points[1], count, cvSize(win_size,win_size), 5, status, 0,
                cvTermCriteria(CV_TERMCRIT_ITER,20,0.03), flags );
            flags |= CV_LKFLOW_PYR_A_READY;
            for( i = k = 0; i < count; i++ )
            {
                if( add_remove_pt )
                {
                    double dx = pt[0].x - points[1][i].x;
                    double dy = pt[0].y - points[1][i].y;

                    if( dx*dx + dy*dy <= 25 )
                    {
                        add_remove_pt = 0;
                        continue;
                    }
                }

                if( !status[i] )
                    continue;

                points[1][k++] = points[1][i];
                cvCircle( image, cvPointFrom32f(points[1][i]), 3, CV_RGB(0,255,0), -1, 8,0);
            }
            count = k;
        }

        if( add_remove_pt && count < MAX_COUNT )
        {
            int i;
            for(i=0;i<add_remove_pt;i++)
                points[1][count++] = cvPointTo32f(pt[i]);
            cvFindCornerSubPix( grey, points[1] + count - 1, 1,
                cvSize(win_size,win_size), cvSize(-1,-1),
                cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03));
            add_remove_pt = 0;
        }

        if( face.detected == 1) {
                cvRectangle( image, face.pt1, face.pt2, CV_RGB(255,0,0), 3, 8, 0 );
        }

        CV_SWAP( prev_grey, grey, swap_temp );
        CV_SWAP( prev_pyramid, pyramid, swap_temp );
        CV_SWAP( points[0], points[1], swap_points );
        need_to_init = 0;
        cvShowImage( "LkDemo", image );
        face.num_frames++;

        c = cvWaitKey(10);
        if( (char)c == 27 )
            break;
        switch( (char) c )
        {
        case 'r':
            need_to_init = 1;
            break;
        case 'c':
            count = 0;
            break;
        case 'n':
            night_mode ^= 1;
            break;
        default:
            ;
        }
    }

    cvReleaseCapture( &capture );
    cvDestroyWindow("LkDemo");

    return 0;
}

#ifdef _EiC
main(1,"lkdemo.c");
#endif
