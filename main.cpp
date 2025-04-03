#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <iostream>
#include <dirent.h>
#include <sys/types.h>
#include <cstring>
#include <vector>
#include <sys/stat.h>
#include "fileManager.h"
#include "utils/ocr.hpp"
#include <cstdio>
#include <future>
#include <fstream>
#include <semaphore.h> // Para semáforos
#include <chrono>
#include <thread>
#include "utils/media.hpp"
const int MAX_CONCURRENT_THREADS = 2;
sem_t semaphore;

std::string performOCR(const std::string &imagePath)
{
    // std::cout << "Realizando OCR en: " << imagePath << std::endl;
    // std::string text = "Texto OCR de " + imagePath; // Placeholder
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // return getTextByImage(imagePath);
    return "test";
}

std::string format_milliseconds(int ms)
{
    int horas = ms / 3600000;
    ms %= 3600000;
    int minutos = ms / 60000;
    ms %= 60000;
    int segundos = ms / 1000;
    int milisegundos = ms % 1000;
    char buffer[20];
    std::sprintf(buffer, "%02d_%02d_%02d_%03d", horas, minutos, segundos, milisegundos);
    return std::string(buffer);
}

void imgExtractor(const std::string &fileName)
{
    string pathVideo = "videos/" + fileName;
    cv::VideoCapture video(pathVideo, cv::VideoCaptureAPIs::CAP_FFMPEG);
    if (!video.isOpened())
    {
        std::cerr << "Error opening video file\n";
        return;
    }

    string folderName = createFolderByFileName(fileName);

    static double fps = video.get(cv::CAP_PROP_FPS);
    int total_frames = static_cast<int>(video.get(cv::CAP_PROP_FRAME_COUNT));
    std::cout << "FPS: " << fps << ", Total Frames: " << total_frames << std::endl;
    static int fpsCut = static_cast<int>(2);
    static int auxTime = 0;
    size_t ventana = 5;
    double factor_umbral = 2.0;
    DetectorCambioBrusco detector(ventana, factor_umbral);

    // Load ONNX model
    cv::dnn::Net net = cv::dnn::readNetFromONNX("models/model_fp16.onnx");
    net.setPreferableBackend(3);
    net.setPreferableTarget(1);

    cv::Mat temporalFrameGray, temporalFrameColor;
    bool contentText = false;
    bool temporalTextState = false;
    std::vector<std::future<std::string>> ocr_futures;
    int counter = 1, temporalCounter = 1, temporalDifference = 0;
    while (counter <= total_frames)
    {
        cv::Mat img;
        video >> img;
        if (img.empty())
            break;

        if (counter % fpsCut == 0)
        {
            cv::Mat frameFull = img.clone();
            contentText = false;
            cv::resize(img, img, cv::Size(224, 224), cv::INTER_AREA);
            cv::Mat frame = img.clone();
            cv::Mat frameAux = img.clone();
            // img.convertTo(img, CV_32F, 1.0 / 255);
            cv::Mat blob = cv::dnn::blobFromImage(img, 1.0 / 255);
            blob = blob.reshape(1, {1, 3, 224, 224});
            // Set input and run forward pass
            net.setInput(blob);
            std::vector<cv::Mat> detections;
            net.forward(detections);

            // Process output
            cv::Mat outputFrame = detections[0].reshape(1, {224, 224}) * 255;
            cv::threshold(outputFrame, outputFrame, 30, 255, cv::THRESH_BINARY);
            cv::morphologyEx(outputFrame, outputFrame, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)));
            cv::erode(outputFrame, outputFrame, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)), cv::Point(-1, -1), 2);
            outputFrame.convertTo(outputFrame, CV_8U);
            // cv::imshow("Output Frame", outputFrame);
            // Find contours
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(outputFrame, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

            if (counter == fpsCut)
            {
                temporalTextState = false;
                temporalFrameGray = outputFrame;
                temporalFrameColor = frameFull;
            }

            // std::vector<std::vector<int>> areas;
            for (const auto &contour : contours)
            {
                cv::Rect rect = cv::boundingRect(contour);
                // if (rect.y > 112 && rect.height > 10)
                // {
                    double area = cv::contourArea(contour);
                    if (area > 112 && area < 112 * 224)
                    {
                        contentText = true;

                        // cv::Rect rect = cv::boundingRect(contour);
                        // cv::rectangle(frame, rect, cv::Scalar(0, 255, 0), 1);
                    }
                // }
            }

            cv::Mat rest;
            cv::absdiff(outputFrame, temporalFrameGray, rest);
            cv::morphologyEx(rest, rest, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)));
            int difference = cv::sum(rest)[0];
            // std::cout << "Difference: " << difference << " time: "<<video.get(cv::CAP_PROP_POS_MSEC) << std::endl;
            // std::cout << "Temporal Difference: " << 1000.0*(double)counter/fps << std::endl;
            // cv::imshow("Difference", rest);
            // cv::imshow("Frame", frame);
            // cv::waitKey(500);
            bool changeDetection=detector.detectar(difference, temporalDifference);
            int temporalTime = (1000*(double)temporalCounter/ fps) - auxTime;
            int temporalTimeC = (1000*(double)counter/ fps) + auxTime;
            if (changeDetection && temporalTextState )
            {

                // cout << "./" + folderName + "/" + format_milliseconds(temporalTime) + "__" + format_milliseconds(temporalTimeC) + ".jpeg" << endl;
                std::string imagePath = "./" + folderName + "/" + format_milliseconds(temporalTime) + "__" + format_milliseconds(temporalTimeC) + ".jpeg";
                cv::imwrite(imagePath, temporalFrameColor); // Espera a que haya un hilo disponible (semáforo)
                sem_wait(&semaphore);

                // Lanza el OCR de forma asíncrona
                std::future<std::string> ocrFuture = std::async(std::launch::async, [imagePath]()
                                                                {
                 std::string result = performOCR(imagePath);
                 sem_post(&semaphore); // Libera un hilo (semáforo)
                 return result; });

                ocr_futures.push_back(std::move(ocrFuture));
            }

            if (changeDetection && contentText)
            {
                temporalFrameColor = frameFull;
                temporalCounter = counter;
            }
            temporalDifference = difference;
            temporalFrameGray = outputFrame;
            temporalTextState = contentText;
        }

        counter++;
    }
    video.release();
    cv::destroyAllWindows();
    // Espera a que todas las tareas de OCR se completen y guarda los resultados
    for (auto &future : ocr_futures)
    {
        std::string ocrText = future.get();
        // std::string imagePath = future.get(); // NO FUNCIONA, el "future" ya se obtuvo.

        // std::string txtPath = imagePath.substr(0, imagePath.size() - 4) + ".txt"; // Reemplaza la extensión .jpeg por .txt
        // std::ofstream outfile(txtPath);
        // outfile << ocrText << std::endl;
        // outfile.close();

        // std::cout << "Texto OCR: " << ocrText << " guardado en " << txtPath << std::endl;
    }
}
int main()
{
    sem_init(&semaphore, 0, MAX_CONCURRENT_THREADS);
    if (existeDirectorio("videos") == false)
    {
        mkdir("videos");
    }
    if (existeDirectorio("subtitles") == false)
    {
        mkdir("subtitles");
    }
    list_files("videos", imgExtractor);
    sem_destroy(&semaphore); // Destruye el semáforo

    return 0;
}
