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
#include <opencv2/imgproc.hpp>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <vector>

#define WIDTH 1280
#define HEIGHT 720

#define ROISIZE 128
#define ROIWIDTH 128
#define ROIHEIGHT 128

#define IDL 0
#define ACQ 1
#define TRK 2

#define FILEID 1

#define THRESHUP 1
#define THRESHDOWN 2

#define XHAIRSIZE 10


//#define WIDTH 1024
//#define HEIGHT 576

using namespace std;
using namespace cv;


struct ROI_t {
    int x;
    int y;
    unsigned char size;
    unsigned char xLSB;
    unsigned char xMSB;
    unsigned char yLSB;
    unsigned char yMSB;
    unsigned char mode;
    unsigned char reqMode;
    unsigned char xTGT;
    unsigned char yTGT;
    unsigned char wTGT;
    unsigned char hTGT;
} ROI;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void TCPSend(int socket, uchar* buffer)
{
    int totalBytesWritten = 0;
    int vecSize = WIDTH*HEIGHT;
    int n;
    while(totalBytesWritten != vecSize)
    {
        n = (int)write(socket,&buffer[totalBytesWritten],vecSize);
        //cout << "Bytes Written: " << n << endl;
        if (n < 0)
            error("ERROR writing to socket");
        totalBytesWritten += n;
    }
}

void TCPReceive(int socket, uchar* buffer)
{
    int totalBytesRead = 0;
    int vecSize = WIDTH*HEIGHT;
    bzero(buffer,vecSize);
    int n;
    totalBytesRead = 0;
    while(totalBytesRead != vecSize)
    {
        n = (int)read(socket,&buffer[totalBytesRead],vecSize - totalBytesRead);
        if (n < 0) error("ERROR reading from socket");
        
        totalBytesRead += n;
    }
}

Mat drawROI(Mat frame, ROI_t ROI)
{
    if(ROI.mode == IDL)
    {
        cv::rectangle(frame, Point(ROI.x,ROI.y), Point(ROI.x+ROI.size, ROI.y+ROI.size), Scalar(0,0,255));
    }
    else if(ROI.mode == ACQ)
    {
        cv::rectangle(frame, Point(ROI.x,ROI.y), Point(ROI.x+ROI.size, ROI.y+ROI.size), Scalar(0,255,255));
    }
    else if(ROI.mode == TRK)
    {
        cv::rectangle(frame, Point(ROI.x,ROI.y), Point(ROI.x+ROI.size, ROI.y+ROI.size), Scalar(0,255,0));
    }
    
    if(ROI.hTGT != 0)
    {
        cout << "TGT - x: " << (int)ROI.xTGT + ROI.x - WIDTH/2 << " y: " << (int)ROI.yTGT + ROI.y - HEIGHT/2 << endl;
        cv::rectangle(frame, Point(ROI.x+ROI.xTGT, ROI.y+ROI.yTGT), Point(ROI.x+ROI.xTGT+ROI.wTGT, ROI.y+ROI.yTGT+ROI.hTGT), Scalar(0,0,255));
    }
    
    cv::line(frame, Point(WIDTH/2, HEIGHT/2-XHAIRSIZE), Point(WIDTH/2, HEIGHT/2+XHAIRSIZE), Scalar(255,255,255));
    cv::line(frame, Point(WIDTH/2-XHAIRSIZE, HEIGHT/2), Point(WIDTH/2+XHAIRSIZE, HEIGHT/2), Scalar(255,255,255));
    
    return frame;
}

void openCVCallback(int event, int x, int y, int flags, void* userdata)
{
    if(event == EVENT_LBUTTONDOWN)
    {
        ROI.x = x - ROIWIDTH/2;
        ROI.y = y - ROIHEIGHT/2;
        if(ROI.mode == IDL) ROI.reqMode = ACQ;
        else ROI.reqMode = IDL;
        
        if(ROI.x < 0) ROI.x = 0;
        if(ROI.x > WIDTH-ROI.size) ROI.x = WIDTH - ROI.size;
        if(ROI.y < 0) ROI.y = 0;
        if(ROI.y > HEIGHT - ROI.size) ROI.y = HEIGHT - ROI.size;
    }
}

int main(int argc, char *argv[])
{
    //-------------------BASIC INIT--------------------------//
    int x,y,k;
    int frame = 0;
    bool showInput = true;
    float fps;
    int fileID = FILEID;
    string filename;
    char keyPressed;
    int abort = 0;
    
    uchar configByte = 0;
    uchar filterType = 0;
    uchar enSobel = 1;
    uchar enBinarise = 1;
    uchar enCombine = 0;
    uchar enCombineFiltering = 0;
    
    uchar adjustThresh =
    
    //-------------------ROI SETUP---------------------------//
    ROI.x = WIDTH/2-ROIWIDTH/2;
    ROI.y = HEIGHT/2-ROIHEIGHT/2;
    ROI.size = ROISIZE;
    ROI.mode = IDL;
    ROI.reqMode = IDL;
    
    //-------------------OPENCV INIT-------------------------//
    uchar sendBuffer[WIDTH*HEIGHT];
    uchar receiveBuffer[WIDTH*HEIGHT];
    Mat inputImage;
    Mat displayImage;
    Vec3b pixel;
    int vecSize = WIDTH*HEIGHT;
    vector<uchar> h_frame_in(vecSize);
    vector<uchar> h_frame_out(vecSize);
    vector<uchar> h_display_frame(vecSize*3);
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
    int sockfd, portno;
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
        
        //-------------Set up config bytes----------------------//
        configByte = (filterType | (enSobel << 2) | (enBinarise << 3) | (enCombine << 4) | (enCombineFiltering << 5) | (abort << 6));
        ROI.xMSB = (ROI.x & 0xff00) >> 8;
        ROI.xLSB = (ROI.x & 0x00ff);
        ROI.yMSB = (ROI.y & 0xff00) >> 8;
        ROI.yLSB = (ROI.y & 0x00ff);
        
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
        
        
        
        //-------------TCP SEND-----------------------------------//
        sendBuffer[0] = configByte;
        sendBuffer[1] = ROI.size;
        sendBuffer[2] = ROI.xMSB;
        sendBuffer[3] = ROI.xLSB;
        sendBuffer[4] = ROI.yMSB;
        sendBuffer[5] = ROI.yLSB;
        sendBuffer[6] = ROI.mode;
        sendBuffer[7] = ROI.reqMode;
        sendBuffer[8] = adjustThresh;
        
        TCPSend(sockfd, sendBuffer);
        
        ROI.xTGT = 0;
        ROI.yTGT = 0;
        ROI.wTGT = 0;
        ROI.hTGT = 0;

        
        //bzero(sendBuffer,vecSize);
        
        //cout << "ConfigByte: " << (int)configByte << endl;
        
        //-----------TCP RECEIVE----------------------------------//
        TCPReceive(sockfd, receiveBuffer);
        for (y = 0; y < HEIGHT; y++)
        {
            for (x = 0; x < WIDTH-1; x++)
            {
                //std::cout << "Accessing Element: " << (y*WIDTH+x) << std::endl;
                h_frame_out[y*WIDTH + x] = receiveBuffer[y*WIDTH + x];
            }
        }
        
        ROI.size = h_frame_out[1];
        ROI.xMSB = h_frame_out[2];
        ROI.xLSB = h_frame_out[3];
        ROI.yMSB = h_frame_out[4];
        ROI.yLSB = h_frame_out[5];
        ROI.mode = h_frame_out[6];
        ROI.xTGT = h_frame_out[7];
        ROI.yTGT = h_frame_out[8];
        ROI.wTGT = h_frame_out[9];
        ROI.hTGT = h_frame_out[10];
        ROI.reqMode = ROI.mode;
        
        
        
        k=0;
        for (y = 0; y < HEIGHT; y++)
        {
            for (x = 0; x < WIDTH; x++)
            {
                h_display_frame[k] = h_frame_out[y*WIDTH+x];
                k++;
                h_display_frame[k] = h_frame_out[y*WIDTH+x];
                k++;
                h_display_frame[k] = h_frame_out[y*WIDTH+x];
                k++;
            }
        }
        
        
        //-----------DISPLAY FRAME--------------------------------//

        if(showInput == false)
        {
            inputImage = Mat(HEIGHT, WIDTH, CV_8UC3, (float*)h_display_frame.data());
        }
        
        displayImage = drawROI(inputImage, ROI);
        
        
        namedWindow("ReceivedImage", WINDOW_AUTOSIZE);
        setMouseCallback("ReceivedImage", openCVCallback, NULL);
        imshow("ReceivedImage", displayImage);
        
        ROI.x = (int)ROI.xMSB*256 + (int)ROI.xLSB;
        ROI.y = (int)ROI.yMSB*256 + (int)ROI.yLSB;
        
        std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();
        fps = 1000000/duration;

//        cout << "ROI - x: " << ROI.x << " y: " << ROI.y << " size: " << (int)ROI.size << endl;
//        cout << "Mode: " << (int)ROI.mode << " ReqMode: " << (int)ROI.reqMode << endl;
        
        adjustThresh = 0;
        
        keyPressed = (char)waitKey(10);
        if(frame > 3)
        {
            switch(keyPressed)
            {
                case(102): //102 = f
                    filterType = (filterType+1) % 4;
                    cout << "Filter Type: " << (int)filterType << endl;
                    break;
                case(101): //101 = e
                    enSobel = !enSobel;
                    cout << "Edge Detection: " << (int)enSobel << endl;
                    break;
                case(98): //98 = b
                    enBinarise = !enBinarise;
                    cout << "Binarisation: " << (int)enBinarise << endl;
                    break;
                case(99): //99 = c
                    enCombine = !enCombine;
                    cout << "Combine: " << (int)enCombine << endl;
                    break;
                case(118): //118 = v
                    enCombineFiltering = !enCombineFiltering;
                    cout << "Combine Filtering: " << (int)enCombineFiltering << endl;
                    break;
                case(27): //27 = escape
                    abort = 1;
                    break;
                case(112): //112 = p
                    keyPressed = 0;
                    while(keyPressed != 112) keyPressed = waitKey(10);
                    break;
                case(119): //119 = w
                    while(keyPressed == 119){
                        keyPressed = (char)waitKey(1);
                        if(ROI.y > 0) ROI.y-=10;
                    }
                    break;
                case(115): //115 = s
                    while(keyPressed == 115){
                        keyPressed = (char)waitKey(1);
                        if(ROI.y < HEIGHT) ROI.y+=10;
                    }
                    break;
                case(97): //97 = a
                    while(keyPressed == 97){
                        keyPressed = (char)waitKey(1);
                        if(ROI.x > 0) ROI.x-=10;
                    }
                    break;
                case(100): //100 = d
                    while(keyPressed == 100){
                        keyPressed = (char)waitKey(1);
                        if(ROI.x < WIDTH) ROI.x+=10;
                    }
                    break;
                case(13): //13 = return
                    if(ROI.mode == IDL) ROI.reqMode = ACQ;
                    else if(ROI.mode == TRK || ROI.mode == ACQ) ROI.reqMode = IDL;
                    break;
                case(105)://105 = i
                    showInput = !showInput;
                    break;
                case(61): //61 = =
                    adjustThresh = THRESHUP;
                    break;
                case(45):
                    adjustThresh = THRESHDOWN;
                    break;
                default:
                    break;
            }
        }
        //--------------------------------------------------------//
        //if(abort) break;
        frame++;

        
    }
    close(sockfd);
    while(1) x = 0;
    return 0;
}
