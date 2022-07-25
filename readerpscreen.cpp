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

    std::string nomeJan = "USD";

    memset(&search, 0, sizeof(xdo_search_t));
    search.max_depth = -1;
    search.require = xdo_search::SEARCH_ANY;
    search.searchmask |= SEARCH_NAME;
    search.winname = nomeJan.c_str();
    search.searchmask |= SEARCH_CLASS;
    search.winclass = nomeJan.c_str();
    search.searchmask |= SEARCH_CLASSNAME;
    search.winclassname = nomeJan.c_str();


    //free(list);
    usleep(100*1000);
    xdo_search_windows(xdo, &search, &list_win, &nwindows);
    usleep(50*1000);
    for (int k = 0; k < 2; ++k)
        for (uint i = 0; i < nwindows; i++) {
            usleep(50*1000);
            xdo_set_window_size(xdo, list_win[i], s_cortes[i].width, s_cortes[i].height, 0);
            usleep(50*1000);
            if (s_cortes[i].tl().y == 30)
                xdo_move_window(xdo,list_win[i],s_cortes[i].tl().x,0);
            else
                xdo_move_window(xdo,list_win[i],s_cortes[i].tl().x,s_cortes[i].tl().y);
            usleep(50*1000);
        }

    naipes_ref.push_back(cv::imread("paus.png",cv::IMREAD_GRAYSCALE));
    ordem_naipes.push_back('P');
    naipes_ref.push_back(cv::imread("ouros.png",cv::IMREAD_GRAYSCALE));
    ordem_naipes.push_back('O');
    naipes_ref.push_back(cv::imread("copas.png",cv::IMREAD_GRAYSCALE));
    ordem_naipes.push_back('C');
    naipes_ref.push_back(cv::imread("espadas.png",cv::IMREAD_GRAYSCALE));
    ordem_naipes.push_back('E');

    bool showImages = false;
    for (int j = 1; j <= 13; j++) {
        cv::String dirname = "treino/"+std::to_string(j)+"/imgs";
        //std::cout << dirname << std::endl;
        std::vector< cv::String > files;
        cv::glob( dirname, files );
        for ( size_t i = 0; i < files.size(); ++i )
        {
            cv::Mat img = cv::imread( files[i] ); // load the image
            if ( img.empty() )
            {
                std::cout << files[i] << " is invalid!" << std::endl; // invalid image, skip it.
                continue;
            }
            if ( showImages )
            {
                imshow( "image", img );
                cv::waitKey(200);
            }
            ordem_cartas.push_back(j);
            cartas_ref.push_back(img.clone());
        }

    }

    hog_knn.Load_Imgs_Label(cartas_ref,ordem_cartas);
    hog_knn.Train();

    jogadas.push_back(cv::imread("jogadas/aum.jpg"));
    jogadas.push_back(cv::imread("jogadas/des.jpg"));
    jogadas.push_back(cv::imread("jogadas/pag.jpg"));
    jogadas.push_back(cv::imread("jogadas/apo.jpg"));

    ref_DL = cv::imread("DL.png",cv::IMREAD_GRAYSCALE);

    Jogador tmpJ;
    tmpJ.eDealer = 0;
    tmpJ.jogada = -1;
    tmpJ.jogando = False;
    tmpJ.pos = -1;
    tmpJ.centro = cv::Point(0,0);
    tmpJ.ref = cv::Rect();

    for(uint i =0;i<6;i++){
        jogadores.push_back(tmpJ);
        jogadores[i].pos = i;
    }

    tam_jogador = cv::Size(106,55);
    jogadores[0].centro = cv::Point(268+(int(tam_jogador.width/2)),233+(int(tam_jogador.height/2)));
    jogadores[1].centro = cv::Point(70+(int(tam_jogador.width/2)),167+(int(tam_jogador.height/2)));
    jogadores[2].centro = cv::Point(88+(int(tam_jogador.width/2)),55+(int(tam_jogador.height/2)));
    jogadores[3].centro = cv::Point(268+(int(tam_jogador.width/2)),20+(int(tam_jogador.height/2)));
    jogadores[4].centro = cv::Point(450+(int(tam_jogador.width/2)),55+(int(tam_jogador.height/2)));
    jogadores[5].centro = cv::Point(464+(int(tam_jogador.width/2)),167+(int(tam_jogador.height/2)));

    jogadores[0].ref = cv::Rect(268,233,tam_jogador.width,tam_jogador.height);
    jogadores[1].ref = cv::Rect(70,167,tam_jogador.width,tam_jogador.height);
    jogadores[2].ref = cv::Rect(88,55,tam_jogador.width,tam_jogador.height);
    jogadores[3].ref = cv::Rect(268,20,tam_jogador.width,tam_jogador.height);
    jogadores[4].ref = cv::Rect(450,55,tam_jogador.width,tam_jogador.height);
    jogadores[5].ref = cv::Rect(464,167,tam_jogador.width,tam_jogador.height);

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

        cv::Mat img = ImageFromDisplay(Pixels, Width, Height, Bpp);//ImageFromDisplay(Pixels, Width, Height, Bpp);
        lock_s_img.lock();
        if(s_cortes.size() > 0){
            res_img.clear();
            for (size_t i = 0; i < s_cortes.size() ; i++) {
                res_img.push_back(img(s_cortes[i]));
            }
        }
        s_img = img;
        lock_s_img.unlock();

        usleep(35*1000);
    }
    usleep(30*1000);
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


std::vector<carta> ReaderPscreen::Get_cartas_MT(cv::Mat img,int index){

    cv::Mat img_busca;
    cv::cvtColor(img,img_busca,cv::COLOR_BGR2GRAY);

    std::vector<carta>cartas;

    for (size_t np = 0; np < naipes_ref.size(); np++) {

        cv::Mat resM;
        cv::matchTemplate(img_busca,naipes_ref[np],resM,cv::TM_SQDIFF_NORMED);//CV_TM_CCOEFF_NORMED CV_TM_SQDIFF_NORMED

        cv::Mat res_th;
        cv::threshold(resM,res_th,0.05,255.0,cv::THRESH_BINARY_INV);
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
            carta tmp_c;
            tmp_c.tipo = ordem_naipes[np];
            tmp_c.pos = boundRect.tl();
            tmp_c.num = -1;
            cartas.push_back(tmp_c);
        }
    }

    struct {
        bool operator()(carta a, carta b) const { return a.pos.x < b.pos.x; }
    } customLess;
    std::sort(cartas.begin(), cartas.end(), customLess);


    for (size_t i = 0; i < cartas.size(); i++){
        cv::Rect rc_ref = cv::Rect(cartas[i].pos.x-2,cartas[i].pos.y-25,18,22);

        //cv::imwrite("num_"+std::to_string(c_sv)+"_"+std::to_string(index)+"_"+std::to_string(i)+"_"+std::to_string(1)+".jpg",img(rc_ref));
        //usleep(50*1000);
        //std::cout<<"num_"+std::to_string(c_sv)+"_"+std::to_string(index)+"_"+std::to_string(i)+"_"+std::to_string(0)+".jpg"<<std::endl;

        cv::Mat ref = img_busca(rc_ref);

        for (size_t j = 0; j < cartas_ref.size(); j++){
            cartas[i].num = hog_knn.Exec(ref);
        }
        cv::rectangle(img,rc_ref,cv::Scalar(0,255,0));
    }


//    for (size_t i = 0; i < cartas.size(); i++){
//        std::cout<<"|"<<cartas[i].tipo<<" : "<<cartas[i].num<<"|";
//    }
//    std::cout<<std::to_string(index)<<"|"<<std::endl;

    return cartas;

}

bool ReaderPscreen::Jogando(cv::Mat img){

    cv::Mat img_barra = img(cv::Rect(10,img.rows-8,img.cols-30,4));
    cv::Mat barra_hsv;
    cv::cvtColor(img_barra,barra_hsv,cv::COLOR_BGR2HSV);
    cv::Mat hsv_th;
    cv::inRange(barra_hsv,cv::Scalar(0,100,100),cv::Scalar(255,255,255),hsv_th);
    //cv::imshow("bara",img_barra);
    //cv::imshow("bara_th",hsv_th);

    if(cv::countNonZero(hsv_th)> 10) return true;

    return false;

}

std::vector<carta> ReaderPscreen::Get_fl(cv::Mat img,cv::Rect ref,int index){

    cv::Mat img_toRead = img(ref);

    std::vector<carta> res = Get_cartas_MT(img_toRead,index);

    //cv::imshow("floop "+std::to_string(index),img_toRead);

    return  res;

}

int ReaderPscreen::Find_DL(cv::Mat img){

    cv::Mat img_busca;
    cv::cvtColor(img,img_busca,cv::COLOR_BGR2GRAY);
    double minVal; double maxVal; cv::Point minLoc; cv::Point maxLoc;

    cv::Mat resM;
    cv::matchTemplate(img_busca,ref_DL,resM,cv::TM_SQDIFF_NORMED);
    cv::minMaxLoc( resM, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat() );
    //cv::rectangle( img, cv::Rect(minLoc.x,minLoc.y,ref_DL.cols,ref_DL.rows), cv::Scalar(255,0,255), 2, 8, 0 );

    struct Jdist
    {
        double dist;
        int pos;
    };std::vector<Jdist> tmpJ;
    for (size_t i = 0; i < jogadores.size(); i++) {
        Jdist tmp;
        tmp.dist = std::sqrt(std::pow(jogadores[i].centro.x-minLoc.x,2)+std::pow(jogadores[i].centro.y-minLoc.y,2));
        tmp.pos = jogadores[i].pos;
        tmpJ.push_back(tmp);
    }
    struct {
        bool operator()(Jdist a, Jdist b) const { return a.dist < b.dist; }
    } customLess;
    std::sort(tmpJ.begin(), tmpJ.end(), customLess);

    jogadores[tmpJ[0].pos].eDealer = true;

    return tmpJ[0].pos;

}

std::vector<cv::Mat> ReaderPscreen::Get_jogadores(int index){

    std::vector<cv::Mat> saida;

    for (size_t i = 0; i < jogadores.size(); i++) {
        saida.push_back(res_img[index](jogadores[i].ref));
    }

    return saida;

}

void ReaderPscreen::TesteHOG(){

    HOG_Detect teste;

    teste.Load_Imgs_Label(cartas_ref,ordem_cartas);
    teste.Train();
    teste.Exec(cartas_ref);
}
