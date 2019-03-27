/**
 * Adaptive Cruise Control
 * Client - udoo controls the
 *  car by comunicating with
 *  an arduino program
 * 
 */

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

/* Ratio for the resized image */
#define RATIO 0.5

using namespace cv;
using namespace std;

char response[4];           /* response from server */
char str_newline[] = "\n";  /* just to write it on a serial */

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int set_interface_attribs(int fd, int speed, int parity)
{
        struct termios tty;

        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0) {
            printf ("error %d from tcgetattr", errno);
            return -1;
        }
        cfsetospeed(&tty, speed);
        cfsetispeed(&tty, speed);

        /* 8 bit chars */
        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;

        /* disable IGNBRK for mismatched speed tests;
         * otherwise receive break as \000 chars
         */
        tty.c_iflag &= ~IGNBRK; /* ignore break signal */
        tty.c_lflag = 0;        /**
                                 * no signaling chars, no echo,
                                 * no canonical processing
                                 */
        tty.c_oflag = 0;        /* no remapping, no delays */
        tty.c_cc[VMIN]  = 0;    /* read doesn't block */
        tty.c_cc[VTIME] = 5;    /* 0.5 seconds read timeout */

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); /* shut off xon/xoff ctrl */

        /**
         * ignore modem controls,
         * enable reading
         */
        tty.c_cflag |= (CLOCAL | CREAD);

        tty.c_cflag &= ~(PARENB | PARODD);  /*  shut off parity */
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0) {
            printf ("error %d from tcsetattr", errno);
            return -1;
        }

        return 0;
}

void set_blocking (int fd, int should_block)
{
        struct termios tty;

        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0) {
            printf ("error %d from tggetattr", errno);
            return;
        }

        tty.c_cc[VMIN]  = should_block ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
            printf ("error %d setting term attributes", errno);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];

     if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }

    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        error("ERROR no such host\n");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    VideoCapture cap(1);
    Mat image;
    int imgSize, bytes, fd;
    char portname[] = "/dev/ttyMCC";

    if (!cap.isOpened())
        return -1;

    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
        error("ERROR opening ttyMCC");

    set_interface_attribs(fd, B115200, 0);
    set_blocking(fd, 0);

    for (;;) {

        /* Preparing a new frame */
        cap >> image;
        resize(image, image, Size(image.cols * RATIO, image.rows * RATIO), 0, 0, CV_INTER_LINEAR);
        imgSize = image.total() * image.elemSize();

        bytes = send(sockfd, image.data, imgSize, 0);

        for (int i = 0; i < 4; i += bytes)
            if ((bytes = recv(sockfd, &response[i], 4 - i, 0)) == -1)
                error("ERROR on recv");

        /* This way communicates with arduino program */
        for (int i = 0; response[i] != '\0'; ++i)
            write(fd, &response[i], 1);
        write(fd, str_newline, 1);

        if (waitKey(30) >= 0)
            break;
    }

    close(sockfd);
    return 0;
}
