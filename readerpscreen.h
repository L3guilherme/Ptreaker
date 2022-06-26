#ifndef READERPSCREEN_H
#define READERPSCREEN_H

#include <vector>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <thread>

class ReaderPscreen
{
public:
    cv::Mat s_img;
    ReaderPscreen();
    cv::Mat ImageFromDisplay(std::vector<uint8_t>& Pixels, int& Width, int& Height, int& BitsPerPixel);
    void StopCap();
    void RunContCap();
    void *CapLoop(void);
    static void* CallCap(void *arg){return ((ReaderPscreen*)arg)->CapLoop();}

private:
    //bool cap;
};

#endif // READERPSCREEN_H
