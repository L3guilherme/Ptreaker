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

#include <mutex>

extern "C" {
#include <xdo.h>
}

class ReaderPscreen
{
public:
    ReaderPscreen();
    void Config(std::vector<cv::Rect> s_cut);
    void StopCap();
    void RunContCap();
    cv::Mat GetScreen();
    std::vector<cv::Mat> GetCuts();
    std::vector<std::string> Get_fl(cv::Mat img, cv::Rect ref);

private:
    static void* CallCap(void *arg){return ((ReaderPscreen*)arg)->CapLoop();}
    void *CapLoop(void );
    cv::Mat ImageFromDisplay(std::vector<uint8_t>& Pixels, int& Width, int& Height, int& BitsPerPixel);
    std::vector<cv::Mat> naipes_ref;
    std::vector<char>ordem_naipes;
    void Get_cartas_MT(cv::Mat img);
};

#endif // READERPSCREEN_H
