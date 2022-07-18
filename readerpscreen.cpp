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
Window *list_win = NULL;


ReaderPscreen::ReaderPscreen()
{
    cap = false;
    s_img = cv::Mat();

    std::cout<<"Init OK"<<std::endl;

}

void ReaderPscreen::Config(std::vector<cv::Rect>s_cut)
{
    s_cortes = s_cut;
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
    xdo_search_windows(xdo, &search, &list_win, &nwindows);
    usleep(50*1000);
    for (int k = 0; k < 2; ++k)
        for (uint i = 0; i < nwindows; i++) {
            usleep(50*1000);
            xdo_set_window_size(xdo, list_win[i], s_cortes[i].width, s_cortes[i].height, 0);
            usleep(50*1000);
            xdo_move_window(xdo,list_win[i],s_cortes[i].tl().x,s_cortes[i].tl().y);
            usleep(50*1000);
        }

    naipes_ref.push_back(cv::imread("paus.png",cv::IMREAD_GRAYSCALE));
    ordem_naipes.push_back("P");
    naipes_ref.push_back(cv::imread("ouros.png",cv::IMREAD_GRAYSCALE));
    ordem_naipes.push_back("O");
    naipes_ref.push_back(cv::imread("copas.png",cv::IMREAD_GRAYSCALE));
    ordem_naipes.push_back("C");
    naipes_ref.push_back(cv::imread("espadas.png",cv::IMREAD_GRAYSCALE));
    ordem_naipes.push_back("E");


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
        usleep(10*1000);

    }
    std::cout<<"FIM Cap"<<std::endl;
    return nullptr;
}

void ReaderPscreen::RunContCap(){

    cap = true;

    if(cap){
        pthread_t tidImg;
        int resultImg;
        resultImg = pthread_create(&tidImg, nullptr, ReaderPscreen::CallCap, this);
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

void ReaderPscreen::Get_cartas_MT(cv::Mat img){
    cv::Mat resM;
    std::vector<cv::Point>pos_cd;
    std::vector<cv::String>tipo_cd;

    cv::Mat img_busca;
    cv::cvtColor(img,img_busca,cv::COLOR_BGR2GRAY);

    for (size_t np = 0; np < naipes_ref.size(); np++) {

        cv::matchTemplate(img_busca,naipes_ref[np],resM,cv::TM_SQDIFF_NORMED);//CV_TM_CCOEFF_NORMED CV_TM_SQDIFF_NORMED

        cv::Mat res_th;
        cv::threshold(resM,res_th,0.1,255.0,cv::THRESH_BINARY_INV);
        res_th.convertTo(res_th,CV_8UC1);

        std::vector<std::vector<cv::Point>>contours;
        cv::findContours(res_th.clone(), contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

        for( size_t i = 0; i < contours.size(); i++ )
        {
            std::vector<cv::Point> contours_poly;
            cv::Rect boundRect;
            cv::approxPolyDP( contours[i], contours_poly, 3, true );
            boundRect = cv::boundingRect( contours_poly );
            cv::rectangle(img,cv::Rect(boundRect.tl().x,boundRect.tl().y,15,15),cv::Scalar(255,0,0));
            pos_cd.push_back(boundRect.tl());
            tipo_cd.push_back(ordem_naipes[np]);

        }

    }


//    struct {
//        bool operator()(int a, int b) const { return a < b; }
//    } customLess;
//    std::sort(s.begin(), s.end(), customLess);


    //cv::Point centro= cv::Point( minLoc.x  , minLoc.y  );
    //cv::circle(img,centro,3,cv::Scalar(255,255,0),-1);

}

std::vector<std::string> ReaderPscreen::Get_fl(cv::Mat img,cv::Rect ref){

    cv::Mat img_toRead = img(ref);

    Get_cartas_MT(img_toRead);

    cv::imshow("floop",img_toRead);

    std::vector<std::string>res;
    return  res;

}
