#include "hog_detect.h"

using namespace cv;
using namespace cv::ml;
using namespace std;

vector< Mat > gradient_lst;
std::vector<int> labels_lst;
Ptr< SVM > g_svm;


HOG_Detect::HOG_Detect()
{

}

Mat HOGImage(Mat srcImg, HOGDescriptor &hog,
             int imgScale, float vecScale,
             bool drawRect)
{
    Mat hogImg;
    resize(srcImg, hogImg, Size(srcImg.cols * imgScale, srcImg.rows * imgScale));

    if (srcImg.channels() == 1) {
        cvtColor(hogImg, hogImg, COLOR_GRAY2BGR);
    } else {
        cvtColor(srcImg, srcImg, COLOR_BGR2GRAY);
    }


    // compute HOG
    vector<float> descriptors;
    hog.compute(srcImg, descriptors);

    /* Ref: Fast Calculation of Histogram of Oriented Gradient Feature
     *      by Removing Redundancy in Overlapping Block
     */
    // count in the window
    int numCellsX = hog.winSize.width / hog.cellSize.width;
    int numCellsY = hog.winSize.height / hog.cellSize.height;
    int numBlocksX = (hog.winSize.width - hog.blockSize.width
                      + hog.blockStride.width) / hog.blockStride.width;
    int numBlocksY = (hog.winSize.height - hog.blockSize.height
                      + hog.blockStride.height) / hog.blockStride.height;

    // count in the block
    int numCellsInBlockX = hog.blockSize.width / hog.cellSize.width;
    int numCellsInBlockY = hog.blockSize.height / hog.cellSize.height;

    int sizeGrads[] = {numCellsY, numCellsX, hog.nbins};
    Mat gradStrengths(3, sizeGrads, CV_32F, Scalar(0));
    Mat cellUpdateCounter(numCellsY, numCellsX, CV_32S, Scalar(0));

    float *desPtr = &descriptors[0];
    for (int bx = 0; bx < numBlocksX; bx++) {
        for (int by = 0; by < numBlocksY; by++) {
            for (int cx = 0; cx < numCellsInBlockX; cx++) {
                for (int cy = 0; cy < numCellsInBlockY; cy++) {
                    int cellX = bx + cx;
                    int cellY = by + cy;
                    int *cntPtr = &cellUpdateCounter.at<int>(cellY, cellX);
                    float *gradPtr = &gradStrengths.at<float>(cellY, cellX, 0);
                    (*cntPtr)++;
                    for (int bin = 0; bin < hog.nbins; bin++) {
                        float *ptr = gradPtr + bin;
                        *ptr = (*ptr * (*cntPtr - 1) + *(desPtr++)) / (*cntPtr);
                    }
                }
            }
        }
    }

    const float PI = 3.1415927;
    const float radRangePerBin = PI / hog.nbins;
    const float maxVecLen = min(hog.cellSize.width,
                                hog.cellSize.height) / 2  * vecScale;

    for (int cellX = 0; cellX < numCellsX; cellX++) {
        for (int cellY = 0; cellY < numCellsY; cellY++) {
            Point2f ptTopLeft = Point2f(cellX * hog.cellSize.width,
                                        cellY * hog.cellSize.height);
            Point2f ptCenter = ptTopLeft + Point2f(hog.cellSize) / 2;
            Point2f ptBottomRight = ptTopLeft + Point2f(hog.cellSize);

            if (drawRect) {
                rectangle(hogImg,
                          ptTopLeft * imgScale,
                          ptBottomRight * imgScale,
                          CV_RGB(100, 100, 100),
                          1);
            }

            for (int bin = 0; bin < hog.nbins; bin++) {
                float gradStrength = gradStrengths.at<float>(cellY, cellX, bin);
                // no line to draw?
                if (gradStrength == 0)
                    continue;

                // draw the perpendicular line of the gradient
                float angle = bin * radRangePerBin + radRangePerBin / 2;
                float scale = gradStrength * maxVecLen;
                Point2f direction = Point2f(sin(angle), -cos(angle));
                line(hogImg,
                     (ptCenter - direction * scale) * imgScale,
                     (ptCenter + direction * scale) * imgScale,
                     CV_RGB(50, 50, 255),
                     1);
            }
        }
    }
    return hogImg;
}


void convert_to_ml( const vector< Mat > & train_samples, Mat& trainData )
{
    //--Convert data
    const int rows = (int)train_samples.size();
    const int cols = (int)std::max( train_samples[0].cols, train_samples[0].rows );
    Mat tmp( 1, cols, CV_32FC1 ); //< used for transposition if needed
    trainData = Mat( rows, cols, CV_32FC1 );

    for( size_t i = 0 ; i < train_samples.size(); ++i )
    {
        CV_Assert( train_samples[i].cols == 1 || train_samples[i].rows == 1 );

        if( train_samples[i].cols == 1 )
        {
            transpose( train_samples[i], tmp );
            tmp.copyTo( trainData.row( (int)i ) );
        }
        else if( train_samples[i].rows == 1 )
        {
            train_samples[i].copyTo( trainData.row( (int)i ) );
        }
    }
}

vector< float > get_svm_detector( const Ptr< SVM >& svm )
{
    // get the support vectors
    Mat sv = svm->getSupportVectors();
    const int sv_total = sv.rows;
    // get the decision function
    Mat alpha, svidx;
    double rho = svm->getDecisionFunction( 0, alpha, svidx );

    CV_Assert( alpha.total() == 1 && svidx.total() == 1 && sv_total == 1 );
    CV_Assert( (alpha.type() == CV_64F && alpha.at<double>(0) == 1.) ||
               (alpha.type() == CV_32F && alpha.at<float>(0) == 1.f) );
    CV_Assert( sv.type() == CV_32F );

    vector< float > hog_detector( sv.cols + 1 );
    memcpy( &hog_detector[0], sv.ptr(), sv.cols*sizeof( hog_detector[0] ) );
    hog_detector[sv.cols] = (float)-rho;
    return hog_detector;
}

void load_images( const String & dirname, vector< Mat > & img_lst, bool showImages = false )
{
    vector< String > files;
    glob( dirname, files );

    for ( size_t i = 0; i < files.size(); ++i )
    {
        Mat img = imread( files[i] ); // load the image
        if ( img.empty() )
        {
            cout << files[i] << " is invalid!" << endl; // invalid image, skip it.
            continue;
        }

        if ( showImages )
        {
            imshow( "image", img );
            waitKey( 1 );
        }
        img_lst.push_back( img );
    }
}

void sample_neg( const vector< Mat > & full_neg_lst, vector< Mat > & neg_lst, const Size & size )
{
    Rect box;
    box.width = size.width;
    box.height = size.height;

    srand( (unsigned int)time( NULL ) );

    for ( size_t i = 0; i < full_neg_lst.size(); i++ )
        if ( full_neg_lst[i].cols > box.width && full_neg_lst[i].rows > box.height )
        {
            box.x = rand() % ( full_neg_lst[i].cols - box.width );
            box.y = rand() % ( full_neg_lst[i].rows - box.height );
            Mat roi = full_neg_lst[i]( box );
            neg_lst.push_back( roi.clone() );
        }
}

void computeHOGs( const Size wsize, const vector< Mat > & img_lst, vector< Mat > & gradient_lst, bool use_flip )
{
    HOGDescriptor hog;
    hog.winSize = wsize;
    Mat gray;
    vector< float > descriptors;

    for( size_t i = 0 ; i < img_lst.size(); i++ )
    {
        if ( img_lst[i].cols >= wsize.width && img_lst[i].rows >= wsize.height )
        {

            Rect r = Rect(( img_lst[i].cols - wsize.width ) / 2,
                          ( img_lst[i].rows - wsize.height ) / 2,
                          wsize.width,
                          wsize.height);
            //cvtColor( img_lst[i](r), gray, COLOR_BGR2GRAY );
            gray = img_lst[i](r);

            hog.compute( gray, descriptors);
            gradient_lst.push_back( Mat( descriptors ).clone() );

            if ( use_flip )
            {
                flip( gray, gray, 1 );
                hog.compute( gray, descriptors, Size( 8, 8 ), Size( 0, 0 ) );
                gradient_lst.push_back( Mat( descriptors ).clone() );
            }
        }
    }
}

void HOG_Detect::Load_Imgs_Label(std::vector<Mat> imgs, std::vector<int> labels){


    cv::Size pos_image_size = imgs[0].size();
    bool flip_samples = false;
    clog << "Histogram of Gradients are being calculated for images...";
    computeHOGs( pos_image_size, imgs, gradient_lst, flip_samples );
    clog << "...[done]" << endl;
    labels_lst = labels;

}

void HOG_Detect::Train(){

    Mat train_data;
    convert_to_ml( gradient_lst, train_data );


    clog << "Training SVM...";
    Ptr< SVM > svm = SVM::create();
    /* Default values to train SVM */
    svm->setCoef0( 0.0 );
    svm->setDegree( 3 );
    svm->setTermCriteria( TermCriteria(TermCriteria::MAX_ITER + TermCriteria::EPS, 1000, 1e-3 ) );
    svm->setGamma( 0 );
    svm->setKernel( SVM::LINEAR );
    svm->setNu( 0.5 );
    svm->setP( 0.1 ); // for EPSILON_SVR, epsilon in loss function?
    svm->setC( 0.01 ); // From paper, soft classifier
    svm->setType( SVM::EPS_SVR ); // C_SVC; // EPSILON_SVR; // may be also NU_SVR; // do regression task
    svm->train( train_data, ROW_SAMPLE, labels_lst );
    g_svm = svm;
    clog << "...[done]" << endl;
}

int HOG_Detect::Exec(Mat img_in){

    HOGDescriptor hog;

    hog.setSVMDetector( get_svm_detector( g_svm ) );
    //hog.winSize = img_in.size();

    //imshow("HOG", HOGImage(img_in, hog,1, 1.0, true));

//    vector< Rect > detections;
//    vector< double > foundWeights;

//    hog.detectMultiScale( img_in, detections, foundWeights );

//    for ( size_t j = 0; j < detections.size(); j++ )
//    {
//        Scalar color = Scalar( 0, foundWeights[j] * foundWeights[j] * 200, 0 );
//        rectangle( img_in, detections[j], color, img_in.cols / 400 + 1 );
//    }

    cv::imshow("teste",img_in);

    return 0;
}
