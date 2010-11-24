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
#define WINSIZEX 640.0
#define WINSIZEY 310.0
#define MOV_LIMIT 5

#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

const char* cascade_name = "haarcascade_frontalface_alt.xml";

struct face_s {
    IplImage *img;
    CvPoint pt1, pt2;
    float area;
    int detected;
    int num_frames;
};

CvMemStorage* storage = 0;
CvHaarClassifierCascade* cascade = 0;
IplImage *image = 0, *grey = 0, *prev_grey = 0, *pyramid = 0, *prev_pyramid = 0, *swap_temp;

int win_size = 10;
const int MAX_COUNT = 500;
CvPoint2D32f* points[2] = {0,0}, *swap_points;
char* status = 0;
int count = 0;
int flags = 0;
int add_remove_pt = 0;
CvPoint pt[30];
int midx=0,midy=0,oldmidx=0,oldmidy=0,minx=0,miny=0,maxx=0,maxy=0;
float zscale=1.5,area=0;
double distancias[10];
double distmed,distmedold=-1;

void * detect_face(void *param) {

  struct face_s *face;
  int i;
  int scale = 1;
  IplImage *img;

  face = (struct face_s *) param;

  if(face->img == NULL)
    return;

  img = cvCreateImage( cvGetSize(face->img), 8, 3 );
  cvCopy( face->img, img, 0 );

  CvSeq* faces = cvHaarDetectObjects( img, cascade, storage,
                               1.1, 2, CV_HAAR_DO_CANNY_PRUNING, cvSize(40, 40) );
  cvReleaseImage(&img);

  if(faces->total > 0) {

    face->detected =1;

    CvRect* r = (CvRect*)cvGetSeqElem( faces, 0 );
    //Find the dimensions of the face,and scale it if necessary
    face->pt1.x = r->x*scale;
    face->pt2.x = (r->x+r->width)*scale;
    face->pt1.y = r->y*scale;
    face->pt2.y = (r->y+r->height)*scale;
    face->area = (face->pt2.x - face->pt1.x) * (face->pt2.y - face->pt1.y);

  } else { 
      face->detected=0;
  }

  /* Se detectou a face, adiciona pontos no centro e arredores para tracking */
  if(count < 16 && face->detected == 1) {
    add_remove_pt=0;
    if(count + add_remove_pt < 16)
      pt[add_remove_pt++] = cvPoint( (face->pt1.x+face->pt2.x)/2, (face->pt1.y+face->pt2.y)/2);
    if(count + add_remove_pt < 16)
      pt[add_remove_pt++] = cvPoint( pt[0].x+4 , pt[0].y);
    if(count + add_remove_pt < 16)
      pt[add_remove_pt++] = cvPoint( pt[0].x-4 , pt[0].y);
    if(count + add_remove_pt < 16)
      pt[add_remove_pt++] = cvPoint( pt[0].x , pt[0].y+4);
    if(count + add_remove_pt < 16)
      pt[add_remove_pt++] = cvPoint( pt[0].x , pt[0].y-4);
    if(count + add_remove_pt < 16)
      pt[add_remove_pt++] = cvPoint( pt[0].x+8 , pt[0].y);
    if(count + add_remove_pt < 16)
      pt[add_remove_pt++] = cvPoint( pt[0].x-8 , pt[0].y);
    if(count + add_remove_pt < 16)
      pt[add_remove_pt++] = cvPoint( pt[0].x , pt[0].y+8);
    if(count + add_remove_pt < 16)
      pt[add_remove_pt++] = cvPoint( pt[0].x , pt[0].y-8);
    if(count + add_remove_pt < 16)
      pt[add_remove_pt++] = cvPoint( pt[0].x+4 , pt[0].y+4);
    if(count + add_remove_pt < 16)
      pt[add_remove_pt++] = cvPoint( pt[0].x-4 , pt[0].y-4);
    if(count + add_remove_pt < 16)
      pt[add_remove_pt++] = cvPoint( pt[0].x+4 , pt[0].y+4);
    if(count + add_remove_pt < 16)
      pt[add_remove_pt++] = cvPoint( pt[0].x-4 , pt[0].y-4);
    if(count + add_remove_pt < 16)
      pt[add_remove_pt++] = cvPoint( pt[0].x+8 , pt[0].y+4);
    if(count + add_remove_pt < 16)
      pt[add_remove_pt++] = cvPoint( pt[0].x-8 , pt[0].y-4);
    if(count + add_remove_pt < 16)
      pt[add_remove_pt++] = cvPoint( pt[0].x+8 , pt[0].y+4);
    if(count + add_remove_pt < 16)
      pt[add_remove_pt++] = cvPoint( pt[0].x-8 , pt[0].y-4);

    minx = maxx = pt[0].x;
    miny = maxy = pt[0].y;
    for( i = 0; i < add_remove_pt; i++ ) {
        if(points[1][i].x < minx) minx = points[1][i].x;
        if(points[1][i].x > maxx) maxx = points[1][i].x;
        if(points[1][i].y < miny) miny = points[1][i].y;
        if(points[1][i].y > maxy) maxy = points[1][i].y;
    }
    area = (maxx - minx) * (maxy - miny);

  }

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


int main( int argc, char** argv ) {

    CvCapture* capture = 0;
    pthread_attr_t attr;
    pthread_t thread;
    IplImage *foto,*wind;
    int s,i,k,c;
    struct face_s face = { .detected = 0, .img = NULL, .num_frames = 0 };
    CvSize ROI, foto_size,cam;

    foto = cvLoadImage(argv[1], -1);
    foto_size = cvGetSize(foto);

    /* Init points array */
    for(i=0;i<30;i++) {
        pt[i].x = 0;
        pt[i].y = 0;
    }

    cvNamedWindow( "foto", 1 );

    if( argc == 2 )
        capture = cvCaptureFromCAM( 0 );
    else if( argc == 3 )
        capture = cvCaptureFromAVI( argv[2] );


    if( !capture ) {
        fprintf(stderr,"Could not initialize capturing...\n");
        return -1;
    }

    storage = cvCreateMemStorage(0);
    cascade = (CvHaarClassifierCascade*)cvLoad( cascade_name, 0, 0, 0 );
    cvClearMemStorage( storage );
    if( !cascade ) {
            fprintf( stderr, "ERROR: Could not load classifier cascade\n" );
            return;
    }

    cvNamedWindow( "Window", 1 );
    //cvSetMouseCallback( "Window", on_mouse, 0 );

//    s = pthread_attr_init(&attr);
//    if (s != 0)
//       handle_error_en(s, "pthread_attr_init");
//
//    s = pthread_create(&thread, &attr,
//                       &detect_face, &face);
//    if (s != 0)
//        handle_error_en(s, "pthread_create");

    foto = cvLoadImage(argv[1], -1);

    for(;;)
    {
        IplImage* frame = 0;

        frame = cvQueryFrame( capture );
        cam = cvGetSize(frame);

        if( !frame ) break;

        if( !image ) {
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

        /* If there are only 6 points, detect face */
        if(count < 6 && face.num_frames % 8 == 0)
            detect_face(&face);

        if( count > 0 ) {

            /* Find medium value of the points 
             * Also calculate the area using the edge points */
            midx = midy = 0;
            minx = maxx = points[1][0].x;
            miny = maxy = points[1][0].y;
            for( i = 0; i < count; i++ ) {
                midx += points[1][i].x;
                midy += points[1][i].y;
                if(points[1][i].x < minx) minx = points[1][i].x;
                if(points[1][i].x > maxx) maxx = points[1][i].x;
                if(points[1][i].y < miny) miny = points[1][i].y;
                if(points[1][i].y > maxy) maxy = points[1][i].y;

            }
            midx /= count;
            midy /= count;


            distmed = 0.0;
            int numdist = 0;
            for( i = 0; i < count; i++ ) {
                for( k = i; k < count; k++ ) {
                    if(i==k || status[i] == 0) continue;
                    double dist = sqrt( pow(points[1][i].x-points[1][k].x, 2) + pow(points[1][i].y-points[1][k].y, 2)  );
                    if(!isnan(dist)) {
                    distmed += dist;
                    numdist++;
                    } else {
printf("deu nan: %d(i %d)  %d(k %d) -  %d(i %d)  %d(k %d)\n", (int)points[1][i].x,i,(int)points[1][k].x,k, (int)points[1][i].y,i,(int)points[1][k].y,k);
                    }
                }
            }
            distmed /= numdist;
            printf("distmed %f\n", distmed);

            float newarea = (maxx - minx) * (maxy - miny);
            //printf("area %f   new area %f\n", area, newarea);
            if(area>0)
                if(newarea/area > 0.5 && newarea/area < 3.0) zscale=newarea/area;


            /* If the middle point moved few pixels DO NOT move window */
            if( abs(midx-oldmidx) < 4) midx=oldmidx;
            if( abs(midy-oldmidy) < 4) midy=oldmidy;

            /* Draw middle point in window */
            cvCircle( image, cvPoint(midx,midy), 9, CV_RGB(0,0,255), -1, 8,0);

            wind = cvCreateImage(foto_size, IPL_DEPTH_8U, 3);
            cvCopy(foto, wind, NULL);

            /* Margin X of the foto will be (Number of pixels per one percent)*(Percent of middle point in camera) */
            int pixx = foto_size.width/100*( 99.0 - (float)( (float)midx/cam.width*100.0) ) - WINSIZEX*zscale/2;
            int pixy = foto_size.height/100*(  99.0 - (float)( (float)midy/cam.height*100.0) ) - WINSIZEY*zscale/2;

            /* If we reach the border of the image, DO NOT let move outside */
            if(pixx < 0) pixx = 0;
            if(pixx +WINSIZEX*zscale > foto_size.width ) pixx = foto_size.width - WINSIZEX*zscale;
            if(pixy +WINSIZEY*zscale > foto_size.height ) pixy = foto_size.height - WINSIZEY*zscale;
            if(pixy < 0 ) pixy = 0;

            /* ROI of window */
            cvSetImageROI(wind, cvRect( pixx, pixy , WINSIZEX*zscale , WINSIZEY*zscale ));
            cvShowImage( "foto", wind );
            cvReleaseImage(&wind);

            /* Remove points too far of the medium point */
            for( i = k = 0; i < count; i++ ) {
                if( (int) points[1][i].x-midx > 50 || (int)points[1][i].x-midx < -50)
                    continue;
                if( (int) points[1][i].y-midy > 50 || (int)points[1][i].y-midy < -50)
                    continue;
                points[1][k++] = points[1][i];
            }
            count = k;

            cvCalcOpticalFlowPyrLK( prev_grey, grey, prev_pyramid, pyramid,
                points[0], points[1], count, cvSize(win_size,win_size), 5, status, 0,
                cvTermCriteria(CV_TERMCRIT_ITER,20,0.03), flags );
            flags |= CV_LKFLOW_PYR_A_READY;

            /* Remove untracked points and draw the remaining */
            for( i = k = 0; i < count; i++ ) {

                if( !status[i] )
                    continue;

                points[1][k++] = points[1][i];
                cvCircle( image, cvPointFrom32f(points[1][i]), 3, CV_RGB(0,255,0), -1, 8,0);
            }
            count = k;
        }

        /* Adiciona pontos para tracking */
        if( add_remove_pt && count < MAX_COUNT ) {
            int i;
            count = 0;
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
        cvShowImage( "Window", image );
        face.num_frames++;

        c = cvWaitKey(10);
        if( (char)c == 27 ) break;

        switch( (char) c ) {
        case 'c':
            count = 0;
            detect_face(&face);
            break;
        default:
            ;
        }
    }

    cvReleaseCapture( &capture );
    cvDestroyWindow("LkDemo");

    return 0;
}
