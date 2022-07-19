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

struct carta
{
    char tipo;
    cv::Point pos;
    int num;
};

struct Jogador{
    int eDealer;
    bool jogando;
    int jogada;
    int pos;
    cv::Point centro;
    cv::Rect ref;
};

class ReaderPscreen
{
public:
    ReaderPscreen();
    void Config(std::vector<cv::Rect> s_cut);
    void StopCap();
    void RunContCap();
    cv::Mat GetScreen();
    std::vector<cv::Mat> GetCuts();
    std::vector<carta> Get_fl(cv::Mat img, cv::Rect ref, int index = 0);
    int Find_DL(cv::Mat img);

private:
    static void* CallCap(void *arg){return ((ReaderPscreen*)arg)->CapLoop();}
    void *CapLoop(void );
    cv::Mat ImageFromDisplay(std::vector<uint8_t>& Pixels, int& Width, int& Height, int& BitsPerPixel);

    std::vector<carta> Get_cartas_MT(cv::Mat img);
    std::vector<cv::Mat> naipes_ref;
    std::vector<char>ordem_naipes;
    std::vector<cv::Mat> cartas_ref;
    std::vector<int>ordem_cartas;
    cv::Mat ref_DL;
    std::vector<Jogador> jogadores;
    cv::Size tam_jogador;

};

#endif // READERPSCREEN_H
