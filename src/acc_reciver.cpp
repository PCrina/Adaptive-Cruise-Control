/**
 * Adaptive Cruise Control
 * Server - processing images
 *
 */

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"

/* in inches */
#define KNOWN_DIAMETER 6.2
#define KNOWN_DIST 20.3
#define CALIBRATION_IMG "img1.png"

/* camera resolution */
#define IMG_WIDTH 640
#define IMG_HEIGHT 480

/* resize ratio */
#define RATIO 0.5

using namespace cv;
using namespace std;

vector<Vec3f> circles;  /* Vector of circles detected */
float focal_len;        /* Focal length of the camera */
char response[4];       /* Response for udoo */
int speed;              /* Car speed */
int AVOID = -1;         /* A var used for bonus/non-bonus mode */
bool AVOIDED = false;   /* True when last response was avoid */

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int find_circle(Mat img)
{
    Mat gray;

    /* Clear last detected circles */
    circles.clear();

    /* Applying some filters, easier to detect circles */
    cvtColor(img, gray, CV_BGR2GRAY);
    GaussianBlur(gray, gray, Size(9, 9), 2, 2);

    /* Apply the Hough Transform to find the circles */
    HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, gray.rows/8, 200, 100, 0, 0);

    /* Preparing to find biggest circle */
    int biggest_radius = 0;
    Point biggest_circle_center;
    int biggest_circle_idx = -1;

    /* Draw and select biggest circle detected */
    for(size_t i = 0; i < circles.size(); ++i) {

        Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
        int radius = cvRound(circles[i][2]);

        if (radius > biggest_radius) {
            biggest_radius = radius;
            biggest_circle_center = center;
            biggest_circle_idx = i;
        }
    }

    /* Circle center */
    circle(img, biggest_circle_center, 3, Scalar(0,255,0), -1, 8, 0 );

    /* Circle outline */
    circle(img, biggest_circle_center, biggest_radius, Scalar(0,0,255), 3, 8, 0 );

    return biggest_circle_idx;
}

void find_focal_len()
{
    int idx;
    float per_diameter;
    Mat img;

    img = imread(CALIBRATION_IMG, CV_LOAD_IMAGE_COLOR);
    resize(img, img, Size(img.cols * RATIO, img.rows * RATIO), 0, 0, CV_INTER_LINEAR);
    if(!img.data)
        error("ERROR reading callibration image");

    idx = find_circle(img);
    per_diameter = 2 * cvRound(circles[idx][2]);

    focal_len = (per_diameter * KNOWN_DIST) / KNOWN_DIAMETER;
}

float dist_to_camera(float per_diameter)
{
    return (KNOWN_DIAMETER * focal_len) / per_diameter;
}

void calc_response(float dist)
{
    if (dist > 60) {
        /**
         * Probably decreased a little
         * it depends on battery level
         * for the motors :))
         */
        speed = 140;
        AVOIDED = false;
    } else if (dist > 40) {
        speed = 100;
        AVOIDED = false;
    } else if (!AVOIDED) {
        /** Send on the serial only
         * one AVOID command, don't
         * want to avoid same thing twice
         */
        speed = AVOID;
        AVOID = true;
    }

    /* BONUS Case */
    if (speed == -1)
        sprintf(response, "a");
    else
        sprintf(response, "%d", speed);
}


int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[1024];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    if (argc < 3) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
     }

    if (strcmp(argv[2], "n") == 0)
        AVOID = 0;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
        sizeof(serv_addr)) < 0)
            error("ERROR on binding");

    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd,
        (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");
    bzero(buffer,1024);

    /**
     * Finding the camera focal length
     * to mesaure distance
     */
    find_focal_len();

    Mat image = Mat::zeros(IMG_HEIGHT * RATIO, IMG_WIDTH * RATIO, CV_8UC3);
    int imgSize = image.total() * image.elemSize(), ptr, bytes;
    uchar sockData[imgSize];

    for (;;) {
        for (int i = 0; i < imgSize; i += bytes)
            if ((bytes =
                recv(newsockfd, sockData + i, imgSize - i, 0)) == -1)
                error("ERROR on recv");

        ptr = 0;
        for (int i = 0; i < image.rows; ++i) {
            for (int j = 0; j < image.cols; ++j) {
                image.at<Vec3b>(i,j) = Vec3b(sockData[ptr+0], sockData[ptr+1], sockData[ptr+2]);
                ptr += 3;
            }
        }

        /* Getting circle in the image */
        int idx = find_circle(image);

        /* Getting distance to it, cvRounde(..) is it's radius,
         * multipying by 2 ti get diameter
         */
        float dist = dist_to_camera(2 * cvRound(circles[idx][2]));

        /* Deciding what the car should do */
        calc_response(dist);

        /* Sending the command for the car */
        send(newsockfd, response, 4, 0);

        /* No purpose for this */
        imshow("Preview", image);

        /* To close the program */
        if (waitKey(30) >= 0)
            break;
    }

    close(sockfd);
    return 0;
}
