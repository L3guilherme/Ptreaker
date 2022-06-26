#include "hog_detect.h"

HOG_Detect::HOG_Detect()
{

}

using namespace cv;
using namespace cv::ml;
using namespace std;

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
            cvtColor( img_lst[i](r), gray, COLOR_BGR2GRAY );
            //hog.compute( gray, descriptors, Size( 8, 8 ), Size( 0, 0 ) );
            gradient_lst.push_back( Mat( descriptors ).clone() );
            if ( use_flip )
            {
                flip( gray, gray, 1 );
                //hog.compute( gray, descriptors, Size( 8, 8 ), Size( 0, 0 ) );
                gradient_lst.push_back( Mat( descriptors ).clone() );
            }
        }
    }
}

void HOG_Detect::Train(){
    vector< Mat > pos_lst, full_neg_lst, neg_lst, gradient_lst;
    vector< int > labels;

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
    svm->train( train_data, ROW_SAMPLE, labels );
    clog << "...[done]" << endl;
}
