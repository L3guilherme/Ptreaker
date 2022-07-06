#include "readerpscreen.h"

bool cap;
cv::Mat s_img;
std::mutex lock_s_img;
std::vector<cv::Rect>s_cortes;
std::vector<cv::Mat>res_img;


ReaderPscreen::ReaderPscreen()
{
    cap = false;
    s_img = cv::Mat();

    std::cout<<"Init OK"<<std::endl;

}

void ReaderPscreen::Config(std::vector<cv::Rect>s_cut)
{
    s_cortes = s_cut;

    std::cout<<"Config OK"<<std::endl;

}

void ReaderPscreen::StopCap(){
    cap = false;
}

cv::Mat ReaderPscreen::GetScreen(){
    lock_s_img.lock();
    cv::Mat tmp = s_img.clone();
    lock_s_img.unlock();
    return tmp;
}

std::vector<cv::Mat>ReaderPscreen::GetCuts(){
    lock_s_img.lock();
    std::vector<cv::Mat> tmp = res_img;
    lock_s_img.unlock();
    return tmp;
}

void *ReaderPscreen::CapLoop(void){
    int Width = 0;
    int Height = 0;
    int Bpp = 0;
    std::vector<std::uint8_t> Pixels;
    while(cap){
        lock_s_img.lock();
        cv::Mat img = ImageFromDisplay(Pixels, Width, Height, Bpp);//ImageFromDisplay(Pixels, Width, Height, Bpp);
        if(s_cortes.size() > 0){
            res_img.clear();
            for (size_t i = 0; i < s_cortes.size() ; i++) {
                res_img.push_back(img(s_cortes[i]));
            }
        }
        lock_s_img.unlock();
        s_img = img;
        usleep(128);
    }
    //cv::destroyWindow("Tela");
    std::cout<<"FIM"<<std::endl;
    return nullptr;
}

void ReaderPscreen::RunContCap(){

    cap = true;

    if(cap){
        pthread_t tidImg;
        int resultImg;
        resultImg = pthread_create(&tidImg, 0, ReaderPscreen::CallCap, (void *)1);
        if (resultImg == 0)
            pthread_detach(tidImg);
    }
}

cv::Mat ReaderPscreen::ImageFromDisplay(std::vector<uint8_t>& Pixels, int& Width, int& Height, int& BitsPerPixel)
{
    Display* display = XOpenDisplay(nullptr);
    Window root = DefaultRootWindow(display);

    XWindowAttributes attributes;
    XGetWindowAttributes(display, root, &attributes);

    Width = attributes.width;
    Height = attributes.height;

    XImage* img = XGetImage(display, root, 0, 0 , Width, Height, AllPlanes, ZPixmap);
    BitsPerPixel = img->bits_per_pixel;
    Pixels.resize(Width * Height * 4);

    memcpy(&Pixels[0], img->data, Pixels.size());

    XDestroyImage(img);
    XCloseDisplay(display);

    if (Width && Height)
    {
        cv::Mat img = cv::Mat(Height, Width, BitsPerPixel > 24 ? CV_8UC4 : CV_8UC3, &Pixels[0]);
        return img;
    }

    return cv::Mat();
}
