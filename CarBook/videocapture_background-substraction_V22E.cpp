//
//  videocapture_background-substraction_V22E.cpp
//  CarBook
//
//  Created by Leonid Stepanov on 19/12/2019.
//  Copyright © 2019 Leonid Stepanov. All rights reserved.
//

#include <stdlib.h> //Для очистки экрана
#include <stdio.h>
#include <iostream> //Для работы с клавиатурой
#include <fstream> //Для работы с файловыми потоками
#include <sstream>
#include <string>
#include <time.h>
#include <iostream>
#include <vector>
#include <iterator> // заголовочный файл итераторов
#include <iomanip>
#include <locale.h>// для корректного вывода кириллицы на экран

//#include "tbb/parallel_for.h"

#include "opencv/cv.h"
#include <cvaux.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>

using namespace cv;
using namespace std;
//using namespace tbb;

//#pragma once;
const int FRAME_WIDTH = 1280;
const int FRAME_HEIGHT = 960;

int selectFlag = 0;
int numOfRect = 0;
int keyboard;
int x, y;
int drag = 0;
const int size = 32;
//#define REGIONS_CNT 10
//int region_coordinates[10][10];//координаты регионов, в которых надо определять движение.

bool calibrationMode; //used for showing debugging windows, trackbars etc.

Point point1, point2, pt3[size], pt4[size], a[size], b[size]; /* vertical points of the bounding box */
// pt3.width[size]
Rect rect; /* bounding box */
Mat img, roiImg[size]; /* roiImg - the part of the image in the bounding box */

struct region_coordinates {
    Point pt3;
    Point pt4;
};

string intToString(int number) {
    std::stringstream ss;
    ss << number;
    return ss.str();
}

void mouseHandler(int event, int x, int y, int flags, void* param) {
    if (calibrationMode == true) {
        //IplImage* img = (IplImage*) param;
        
        if (event == CV_EVENT_LBUTTONDOWN && !drag) {
            /* left button clicked. ROI selection begins */
            point1 = Point(x, y);
            point2 = Point(x, y);
            point1 = point2;
            
            drag = 1;
        }
        
        if (event == CV_EVENT_MOUSEMOVE && drag) {
            /* mouse dragged. ROI being selected */
            // Mat img1 = img.clone();
            if ((x <= FRAME_WIDTH - 2) && (y <= FRAME_HEIGHT - 2))
                point2 = Point(x, y);
            if (x >= FRAME_WIDTH - 2)
                point2 = Point(FRAME_WIDTH - 2, y);
            if (y >= FRAME_HEIGHT - 2)
                point2 = Point(x, FRAME_HEIGHT - 2);
            if ((x >= FRAME_WIDTH - 2) && (y >= FRAME_HEIGHT - 2))
                point2 = Point(FRAME_WIDTH - 2, FRAME_HEIGHT - 2);
        }
        
        if (event == CV_EVENT_LBUTTONUP && drag) {
            //if ((x <= FRAME_WIDTH-2) && (y <= FRAME_HEIGHT-2)) point2 = Point(x, y);
            //if (x >= FRAME_WIDTH-2) point2.x = FRAME_WIDTH-2;
            //if (y >= FRAME_HEIGHT-2) point2.y = FRAME_HEIGHT-2;
            //if ((x >= FRAME_WIDTH-2) && (y >= FRAME_HEIGHT-2)) point2 = Point(FRAME_WIDTH-2, FRAME_HEIGHT-2);
            if ((point1.x < point2.x) && (point1.y < point2.y))
                rect = Rect(point1.x, point1.y, abs(x - point1.x),
                            abs(y - point1.y));
            if ((point1.x > point2.x) && (point1.y > point2.y))
                rect = Rect(point2.x, point2.y, abs(x - point1.x),
                            abs(y - point1.y));
            if ((point1.x > point2.x) && (point1.y < point2.y))
                rect = Rect(point2.x, point1.y, abs(x - point1.x),
                            abs(y - point1.y));
            if ((point1.x < point2.x) && (point1.y > point2.y))
                rect = Rect(point1.x, point2.y, abs(x - point1.x),
                            abs(y - point1.y));
            
            roiImg[numOfRect] = img(rect);
            selectFlag = 1;
            drag = 0;
        }
    }
}

int main(int, char**) {
    //setlocale(LC_CTYPE, "russian_Russia.1251"); // для корректного вывода кириллицы на экран
    setlocale(LC_ALL, "Russian");
    
    //initModule_video();
    //setUseOptimized(true);
    //setNumThreads(8);
    bool exit = true;
    
    region_coordinates arr[size];
    region_coordinates tmp;
    
    //Create trackbar to change brightness
    int iSliderValue[size];
    int temp[size];
    bool resetROI = false;
    int width[size];
    int height[size];
    int array[FRAME_WIDTH][FRAME_HEIGHT];
    int sum[FRAME_WIDTH * FRAME_HEIGHT];
    calibrationMode = true;
    
    //--- INITIALIZE VIDEOCAPTURE
    VideoCapture cap;
    // open the default camera using default API
    cap.open(1);
    
    // check if we succeeded
    if (!cap.isOpened()) {
        cerr << "ERROR! Unable to open camera\n";
        return -1;
    }
    //--- GRAB AND WRITE LOOP
    cout << "Start grabbing" << endl
    << "Press 'c' key to toggle Rec or Run mode" << endl
    << "Press 'Space' key to write ROI in array" << endl
    << "Press 'w' key to write ROI in file 'data.dat'" << endl
    << "Press 'r' key to read ROI from file 'data.dat'" << endl
    << "Press 'd' key to delete all ROI from file 'data.dat'" << endl
    << "Press 'q' or 'Q' key to terminate programm" << endl;
    
    //-- Выставляем параметры камеры ширину и высоту кадра в пикселях
    cap.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
    
    Mat fgMaskMOG[size];
    Ptr<BackgroundSubtractor> pMOG[size] = new BackgroundSubtractorMOG();
    //cv::namedWindow("ROI - " +intToString(numOfRect+1));
    while (exit) {
        // Захват кадра изображения камеры
        //cap >> img;
        cap.read(img);
        cvSetMouseCallback("Видео рабочей области", mouseHandler, NULL);
        //cvSetMouseCallback("Image", mouseHandler, NULL);
        if (selectFlag == 1) {
            
            createTrackbar("Порог срабатывания ROI-"+intToString(numOfRect+1), "ROI - " +intToString(numOfRect+1),
                           &iSliderValue[numOfRect], 100);
            // process the image to obtain a mask image.
            pMOG[numOfRect]->operator()(roiImg[numOfRect],
                                        fgMaskMOG[numOfRect], resetROI); // resetROI: 0-работа, 1-сброс яркости ROI
            resizeWindow("ROI - " + intToString(numOfRect + 1), 320, 50);
            // Ставим текущий номер ROI в выделенном прямоугольнике
            putText(roiImg[numOfRect], intToString(numOfRect + 1), Point(5, 15),
                    1, 1, Scalar(0, 255, 255), 1);
            // Подписываем название текущего окна ROI и показываем его
            imshow("ROI - " + intToString(numOfRect + 1), fgMaskMOG[numOfRect]); /* show the image bounded by the box */
            
            //unsigned char I = mat.data <unsigned char> (abs(point2.x-point1.x), abs(point2.y-point1.y));
            
            //width[numOfRect] = abs(point2[numOfRect].x-point1[numOfRect].x);
            //height[numOfRect] = abs(point2[numOfRect].y-point1[numOfRect].y);
            
            width[numOfRect] = abs(point2.x-point1.x);
            height[numOfRect] = abs(point2.y-point1.y);
            
            //int sum(cv::Mat fgMaskMOG[numOfRect]) //image.type() == CV_8UC1
            //{
            int sum = 0;
            for(int y=0; y < fgMaskMOG[numOfRect].rows; ++y)
            {
                for(int x=0; x < fgMaskMOG[numOfRect].cols; ++x)
                    sum += fgMaskMOG[numOfRect].at<unsigned char>(y, x);
            }
            //sum = (sum/rows*cols)*100;
            cout << numOfRect << "  sum: " << sum << "\n";
            //return sum;
            //}
            
        }
        
        // Подписываем режим работы основного окна -Rec, -Run.
        if (calibrationMode == true) {
            
            if (keyboard == 27) // Если 'Esc' отмена текущего ROI
            {
                destroyWindow("ROI - " + intToString(numOfRect + 1));
                numOfRect--;
            }
            
            // Режим записи прямоугольников ROI
            putText(img, "Rec", Point(0, 60), 2, 2, Scalar(0, 255, 255), 2);
            
            // Рисуем текущий прямоугольник ROI
            rectangle(img, point1, point2, CV_RGB(255, 0, 0), 2, 8, 0);
            // Рисуем вспомогательные линии - горизонталь и вертикаль
            line(img, cv::Point(0, point2.y), cv::Point(point2.x, point2.y),
                 CV_RGB(250,0,0), 1, 8);
            line(img, cv::Point(point2.x, point2.y),
                 cv::Point(FRAME_WIDTH, point2.y), CV_RGB(250,0,0), 1, 8);
            line(img, cv::Point(point2.x, 0), cv::Point(point2.x, point2.y),
                 CV_RGB(250,0,0), 1, 8);
            line(img, cv::Point(point2.x, point2.y),
                 cv::Point(point2.x, FRAME_HEIGHT), CV_RGB(250,0,0), 1, 8);
        } else {
            
            putText(img, "Run", Point(0, 60), 2, 2, Scalar(0, 0, 255), 2);
            
        }
        
        // Жмем "ПРОБЕЛ" для записи текущего ROI в массив arr[i]
        if (keyboard == 32) { //<Space>=32 add rectangle to current image
            /*
             printf("%d. rect x=%d\t y=%d\t width=%d\t height=%d\n",
             numOfRect+1,point1.x,point1.y,abs(point2.x-point1.x),abs(point2.y-point1.y));
             */
            // Rect R = boundingRect(roiImg[numOfRect]); // Get bounding box for contour i
            // Mat ROI=img(R); //Set ROI on source image
            // imwrite("cropped.jpg",ROI); //save ROI image
            
            pt3[numOfRect] = point1;
            pt4[numOfRect] = point2;
            
            width[numOfRect] = abs(pt4[numOfRect].x-pt3[numOfRect].x);
            height[numOfRect] = abs(pt4[numOfRect].y-pt3[numOfRect].y);
            
            arr[numOfRect].pt3 = point1;
            arr[numOfRect].pt4 = point2;
            
            // добавляем в конец вектора region_coordinates элементы 4, 3, 1
            //region_coordinates.insert(region_coordinates.end(), pt3[numOfRect]);
            
            cout << numOfRect << ". pt3: " << arr[numOfRect].pt3 << " pt4: "
            << arr[numOfRect].pt4 << "\n";
            
            numOfRect++;
            selectFlag = 0;
        }
        // Рисуем прямоугольники ROI в количестве "numOfRect" и ставим в окне его номер intToString(i+1)
        for (int i = 0; i < numOfRect; i++) {
            pt3[i] = arr[i].pt3;
            pt4[i] = arr[i].pt4;
            rectangle(img, pt3[i], pt4[i], CV_RGB(0, 255, 0), 2, 8, 0);
            //comp_rect[i] = cvRect( 0, 0, size.width, size.height );
            putText(roiImg[i], intToString(i + 1), Point(5, 15), 1, 1,
                    Scalar(0, 255, 255), 1);
        }
        
        // Выводим BGS для всех ROI в режиме 'Run'
        if ((calibrationMode == false) && (selectFlag == 0)) {
            
            for (int i = 0; i < numOfRect; i++) {
                //Mat dst[i];
                createTrackbar("Порог срабатывания ROI-"+intToString(i+1), "ROI - " +intToString(i+1),
                               &iSliderValue[i], 100);
                //createTrackbar("Brightness ROI-"+intToString(i+1), "ROI - " +intToString(i+1), &iSliderValue1, 100);
                
                // process the image to obtain a mask image.
                pMOG[i]->operator()(roiImg[i], fgMaskMOG[i], resetROI);// resetROI: 0-работа, 1-сброс яркости ROI
                // Ставим текущий номер ROI в выделенном прямоугольнике
                //putText(roiImg[i], intToString(i+1), Point(5,15), 1, 1, Scalar(0, 255, 255), 1);
                resizeWindow("ROI - " + intToString(i + 1), 320, 50);
                // Подписываем название текущего окна ROI и показываем его
                imshow("ROI - " + intToString(i + 1), fgMaskMOG[i]); /* show the image bounded by the box */
                //imshow("ROI - " +intToString(i+1), dst[i]); /* show the image Trackbar */
            }
        }
        
        
        // Показываем окно рабочей области изображения
        imshow("Видео рабочей области", img);
        //imshow("Image", img);
        
        //delay 30ms so that screen can refresh.
        //image will not appear without this waitKey() command
        //also use waitKey command to capture keyboard input
        //get the input from the keyboard
        keyboard = waitKey(15);
        //if user presses 'c', toggle calibration mode
        if (keyboard == 99) calibrationMode = !calibrationMode;
        //if user presses 'q' or 'Q' -> exit from programm
        if (keyboard == 'q' || keyboard == 'Q') exit = false;
        //if user presses 's' -> сброс яркости ROI
        if (keyboard == 's') resetROI = !resetROI;
        
        // Запись массива регионов ROI в файл "data.dat"
        if ((keyboard == 'w') && (calibrationMode == true)) { // Запись массива arr[size]  файл "data.dat"
            
            ofstream fout("data.dat", ios::binary);
            for (int i = 0; i < size; ++i) {
                fout.write((char *) (&arr[i]), sizeof(region_coordinates));
            }
            fout.close();
        }
        
        // Запись массива уставок iSliderValue[i] в файл "SliderValue.dat"
        if ((keyboard == 'w') && (calibrationMode == true)) {
            ofstream fout("SliderValue.dat", ios::binary);
            for (int i = 0; i < size; i++) {
                //if(size > numOfRect) iSliderValue[i]=0;
                fout.write((char *) (&iSliderValue[i]), sizeof(iSliderValue));
            }
            fout.close();
        }
        
        // Чтение массива  arr[size] из  файл "data.dat" и вывод в консоль
        if ((keyboard == 'r') && (calibrationMode == true)) {
            Point c;
            c.x = 0;
            c.y = 0;
            numOfRect = 0;
            
            ifstream fin("data.dat", ios::binary);
            cout << "\n";
            for (int i = 0; i < size; i++) {
                fin.read((char *) &tmp, sizeof(region_coordinates));
                a[i] = tmp.pt3;
                b[i] = tmp.pt4;
                
                if ((a[i] == c) && b[i] == c) { // если  ноль
                    i = size; // Выход из цикла "For"
                } else {
                    pt3[i] = tmp.pt3;
                    pt4[i] = tmp.pt4;
                    arr[i].pt3 = tmp.pt3;
                    arr[i].pt4 = tmp.pt4;
                    cout << i << ". pt3: " << tmp.pt3 << " pt4: " << tmp.pt4
                    << "\n";
                    numOfRect = i + 1; //?
                    rect = Rect(pt3[i], pt4[i]);
                    roiImg[i] = img(rect);
                }
            }
            fin.close();
        }
        
        // Чтение массива  массива уставок iSliderValue[i] из файла "SliderValue.dat" и вывод в консоль
        if ((keyboard == 'r') && (calibrationMode == true)) {
            ifstream fin("SliderValue.dat", ios::binary);
            //cout << "\n";
            for (int i = 0; i < size; i++) {
                fin.read((char *) &temp[i], sizeof(iSliderValue[size]));
                iSliderValue[i] = temp[i];
                
                if ((iSliderValue[i] <= 0) || (iSliderValue[i] > 100)) { // если  <=0 или >100
                    i = size; // Выход из цикла "For"
                } else {
                    iSliderValue[i] = temp[i];
                    cout << i << "  iSliderValue: " << temp[i] << "\n";
                }
            }
            fin.close();
        }
        
        // Обнуление файла "data.dat" регионов ROI
        if ((keyboard == 'd') && (calibrationMode == true)) { // Запись нулей в массив arr[size] и в файл "data.dat"
            point1.x = 0;
            point1.y = 0;
            point2.x = 0;
            point2.y = 0;
            ofstream fout("data.dat", ios::binary);
            for (int i = 0; i < size; ++i) {
                
                arr[i].pt3 = point1;
                arr[i].pt4 = point2;
                pt3[i] = point1;
                pt4[i] = point2;
                fout.write((char *) (&arr[i]), sizeof(region_coordinates));
                
            }
            fout.close();
            numOfRect = 0;
            ifstream fin("data.dat", ios::binary);
            cout << "\n";
            //numOfRect = size;
            for (int i = 0; i < size; ++i) {
                fin.read((char *) &tmp, sizeof(region_coordinates));
                cout << i + 1 << ". pt3: " << tmp.pt3 << " pt4: " << tmp.pt4
                << "\n";
                pt3[i] = tmp.pt3;
                pt4[i] = tmp.pt4;
                rect = Rect(pt3[i], pt4[i]);
                roiImg[i] = img(rect);
            }
            fin.close();
        }
        
    }
    cap.release();
    img.release();
    return 0;
}

