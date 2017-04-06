#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <vector>

#define WIDTH 1280
#define HEIGHT 720

//#define WIDTH 1024
//#define HEIGHT 576

using namespace std;
using namespace cv;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    //-------------------BASIC INIT--------------------------//
    int x,y;
    int showInput = 0;
    float fps;
    int fileID = 1;
    string filename;
    
    //-------------------OPENCV INIT-------------------------//
    uchar sendBuffer[WIDTH*HEIGHT];
    uchar receiveBuffer[WIDTH*HEIGHT];
    Mat inputImage;
    Vec3b pixel;
    int vecSize = WIDTH*HEIGHT;
    vector<uchar> h_frame_in(vecSize);
    vector<uchar> h_frame_out(vecSize);
    VideoCapture cap;
    
    
    if(fileID == 0)
    {
        cap.open(0);
        if (!cap.isOpened()) return -1;
    }
    else
    {
        if(fileID == 1) filename = "/Users/ryanyates/Documents/Work/Project/Videos/High with Clouds.mp4";
        else if(fileID == 2) filename = "/Users/ryanyates/Documents/Work/Project/Videos/Low in Front of Trees.mp4";
        else if(fileID == 3) filename = "/Users/ryanyates/Documents/Work/Project/Videos/Mid Height Stable Camera.mp4";
        else if(fileID == 4) filename = "/Users/ryanyates/Documents/Work/Project/Videos/Low with Mixed Background.mp4";
        
        cap.open(filename);
    }
    
    
    
    
    cap.set(CV_CAP_PROP_FRAME_WIDTH, WIDTH);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);
    
    
    
    
    
    
    
    //-------------------TCP CLIENT INIT-----------------------//
    int totalBytesWritten = 0;
    int totalBytesRead = 0;
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
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
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    //printf("Please enter the message: ");
    
    
    
    while(1)
    {
        //-------------CAPTURE FRAME----------------------------//
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        cap >> inputImage;
        
        if(inputImage.empty()) break;
        
        for (y = 0; y < HEIGHT; y++)
        {
            for (x = 0; x < WIDTH; x++)
            {
                pixel = inputImage.at<Vec3b>(y, x);
                h_frame_in[y*WIDTH + x] = (uchar)(0.0722*pixel.val[0] + 0.7152*pixel.val[1] + 0.2126*pixel.val[2]);
                sendBuffer[y*WIDTH + x] = h_frame_in[y*WIDTH + x];
            }
        }
        
        if (showInput == 1)
        {
            inputImage =  Mat(HEIGHT, WIDTH, CV_8U, (float*)h_frame_in.data());
            namedWindow("Sent Image", WINDOW_AUTOSIZE);
            imshow("Sent Image", inputImage);
            //waitKey(17);
        }
        
        //-------------TCP SEND-----------------------------------//
        //bzero(sendBuffer,vecSize);
        //fgets(sendBuffer,WIDTH*HEIGHT-1,stdin);
        totalBytesWritten = 0;
        while(totalBytesWritten != vecSize)
        {
            n = (int)write(sockfd,&sendBuffer[totalBytesWritten],vecSize);
            //cout << "Bytes Written: " << n << endl;
            if (n < 0)
                error("ERROR writing to socket");
            totalBytesWritten += n;
        }
        
        //bzero(sendBuffer,vecSize);
        
        //-----------TCP RECEIVE----------------------------------//
        bzero(receiveBuffer,vecSize);
        totalBytesRead = 0;
        while(totalBytesRead != vecSize)
        {
            n = (int)read(sockfd,&receiveBuffer[totalBytesRead],vecSize - totalBytesRead);
            if (n < 0) error("ERROR reading from socket");
            
            totalBytesRead += n;
        }
        //std::cout << "Frame Received" << std::endl;
        
        //-----------DISPLAY FRAME--------------------------------//
        for (y = 0; y < HEIGHT; y++)
        {
            for (x = 0; x < WIDTH-1; x++)
            {
                //std::cout << "Accessing Element: " << (y*WIDTH+x) << std::endl;
                h_frame_out[y*WIDTH + x] = receiveBuffer[y*WIDTH + x];
            }
        }
        inputImage = Mat(HEIGHT, WIDTH, CV_8U, (float*)h_frame_out.data());
        namedWindow("ReceivedImage", WINDOW_AUTOSIZE);
        imshow("ReceivedImage", inputImage);
        
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();
        fps = 1000000/duration;
        
        std::cout << fps << std::endl;
        
        waitKey(1);
        //--------------------------------------------------------//
        
    }
    close(sockfd);
    while(1) x = 0;
    return 0;
}
