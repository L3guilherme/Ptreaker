#include "readerpscreen.h"
extern "C" {
#include <xdo.h>
}

bool cap;
cv::Mat s_img;
std::mutex lock_s_img;
std::vector<cv::Rect>s_cortes;
std::vector<cv::Mat>res_img;
xdo_t *xdo = xdo_new(NULL);


ReaderPscreen::ReaderPscreen()
{
    cap = false;
    s_img = cv::Mat();

    std::cout<<"Init OK"<<std::endl;

}

void ReaderPscreen::Config(std::vector<cv::Rect>s_cut)
{
    s_cortes = s_cut;
    Window *list = NULL;
    xdo_search_t search;
    unsigned int nwindows;

    memset(&search, 0, sizeof(xdo_search_t));
    search.max_depth = -1;
    search.require = xdo_search::SEARCH_ANY;
    search.searchmask |= SEARCH_NAME;
    search.winname = "USD";
    search.searchmask |= SEARCH_CLASS;
    search.winclass = "USD";
    search.searchmask |= SEARCH_CLASSNAME;
    search.winclassname = "USD";

    //free(list);
    usleep(100*1000);
    xdo_search_windows(xdo, &search, &list, &nwindows);
    usleep(50*1000);
    for (int k = 0; k < 2; ++k)
    for (uint i = 0; i < nwindows; i++) {
        usleep(50*1000);
        xdo_set_window_size(xdo, list[i], s_cortes[i].width, s_cortes[i].height, 0);
        usleep(50*1000);
        xdo_move_window(xdo,list[i],s_cortes[i].tl().x,s_cortes[i].tl().y);
        usleep(50*1000);
    }

    //    xdo_move_mouse(xdo,50,125,0);
    //    usleep(500*1000);
    //    xdo_mouse_down(xdo,CURRENTWINDOW,1);
    //    usleep(200*1000);
    //    xdo_mouse_up(xdo,CURRENTWINDOW,1);
    //    usleep(500*1000);

    //    uchar *name;
    //    int name_len;
    //    int name_type;

    //    xdo_get_window_name(xdo, CURRENTWINDOW, &name, &name_len, &name_type);
    //    std::cout<<name<<std::endl;
    //    std::cout<<name_len<<std::endl;
    //    std::cout<<name_type<<std::endl;


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
