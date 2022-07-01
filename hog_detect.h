#ifndef HOG_DETECT_H
#define HOG_DETECT_H


#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/ml.hpp"
#include "opencv2/objdetect.hpp"
#include "opencv2/videoio.hpp"
#include <iostream>
#include <time.h>


class HOG_Detect
{
public:
    HOG_Detect();
    void Train();
    void Load_Imgs_Label(std::vector<cv::Mat> imgs,std::vector<int>labels);
};

#endif // HOG_DETECT_H
