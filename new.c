// Sample Application to demonstrate how Face detection can be done as a part of your source code.

// Include header files
#include "cv.h"
#include "highgui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>

#define FACE_DETECT_STEP 3

// Create a string that contains the exact cascade name
const char* cascade_name =
   /* "C:/Program Files/OpenCV/data/haarcascades/haarcascade_frontalface_alt.xml";*/
    "haarcascade_frontalface_alt.xml";

struct face_s {
    IplImage *img;
    CvPoint pt1, pt2;
    int area;
    int detected;
};

CvMemStorage* storage = 0;
CvHaarClassifierCascade* cascade = 0;

//void detect_face( int event, int x, int y, int flags, void *param) {
void detect_face (void *param) {

  struct face_s *face;

  face = (struct face_s *) param;
  IplImage *img = face->img;
  int i;
  int scale = 1;
  CvSeq* faces = cvHaarDetectObjects( img, cascade, storage,
                               1.1, 2, CV_HAAR_DO_CANNY_PRUNING, cvSize(40, 40) );

  if(faces->total > 0) face->detected =1;
  else face->detected=0;
  // Loop the number of faces found.
  for( i = 0; i < (faces ? faces->total : 0); i++ ) {
  // Create a new rectangle for drawing the face
    CvRect* r = (CvRect*)cvGetSeqElem( faces, i );

    //Find the dimensions of the face,and scale it if necessary
    face->pt1.x = r->x*scale;
    face->pt2.x = (r->x+r->width)*scale;
    face->pt1.y = r->y*scale;
    face->pt2.y = (r->y+r->height)*scale;
    face->area = (face->pt2.x - face->pt1.x) * (face->pt2.y - face->pt1.y);

    // Draw the rectangle in the input image
    //cvRectangle( img, face->pt1, face->pt2, CV_RGB(255,0,0), 3, 8, 0 );
  }

} //end detect_face

// Main function, defines the entry point for the program.
int main( int argc, char** argv ) {

  if ( argc != 1 )
    printf("./face <foto>\n");
  
    struct face_s face;
    face.detected=0;

    IplImage *frame,*framec,*last=NULL;

    CvCapture* capture = cvCaptureFromCAM( CV_CAP_ANY );
    CvSize ROI = cvGetSize(cvQueryFrame( capture ));

    storage = cvCreateMemStorage(0);
    cascade = (CvHaarClassifierCascade*)cvLoad( cascade_name, 0, 0, 0 );
    // Check whether the cascade has loaded successfully. Else report and error and quit
    cvClearMemStorage( storage );
    if( !cascade ) {
            fprintf( stderr, "ERROR: Could not load classifier cascade\n" );
            return;
    }
    
    int i;
    int num_frames = 0;
    cvNamedWindow( "result", 1 );
    //cvSetMouseCallback( "result", detect_face, (void*) &face);
    while(1) {
        framec = cvQueryFrame( capture );
        if(last == NULL) {
            last = framec;
            continue;
        }
        face.img = framec;

        // Create a sample image
        IplImage *img = framec;

        // Create a new image based on the input image
        //IplImage* temp = cvCreateImage( cvSize(img->width/scale,img->height/scale), 8, 3 );
        IplImage* temp = img;

        // Find whether the cascade is loaded, to find the faces. If yes, then:
	if ( num_frames % FACE_DETECT_STEP == 0 )
	  detect_face( (void*) &face);
	num_frames++;
	
        if( face.detected == 1)
                cvRectangle( img, face.pt1, face.pt2, CV_RGB(255,0,0), 3, 8, 0 );

        // Show the image in the window named "result"
        cvShowImage( "result", img );

 //       cvReleaseImage(&img);
        
        // Destroy the window previously created with filename: "result"
        if( (cvWaitKey(10) & 255) == 27 ) {
            printf("sai\n");
            break;
        }

        last = framec;

    }

    cvReleaseCapture( &capture );
    cvDestroyWindow("result");

    // return 0 to indicate successfull execution of the program
    return 0;
}

