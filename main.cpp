#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/highgui.hpp>

#include "fileManager.h"

// --- Constantes de configuración ---
const int DNN_INPUT_SIZE = 224;
const double DNN_SCALE_FACTOR = 1.0 / 255.0;
const int FRAME_SKIP_RATE = 2; 
const double AREA_THRESHOLD_MIN = 240.0;
const double AREA_THRESHOLD_MAX = 22580.0;
const cv::String MODEL_PATH = "models/model.onnx";
const std::string VIDEOS_DIR = "videos";

// #############################################################################
// ### NUEVAS CONSTANTES PARA MEJORAR LA DETECCIÓN DE CAMBIO DE SUBTÍTULO ###
// #############################################################################
const int TEXT_ABSENCE_PERSISTENCE_THRESHOLD = 3; 

// Umbral de píxeles cambiados en la máscara de 224x224 para considerar un nuevo subtítulo.
// Un valor entre 300-800 suele funcionar bien. Un valor más bajo es más sensible.
// Total de píxeles es 224*224 = 50,176. 500 píxeles es ~1% de cambio.
const int TEXT_CHANGE_PIXEL_THRESHOLD = 500;
// #############################################################################

// (Las funciones auxiliares get_filename_from_path y format_milliseconds no cambian)
std::string get_filename_from_path(const std::string& path) {
    size_t last_slash_pos = path.find_last_of("/\\");
    if (std::string::npos != last_slash_pos) {
        return path.substr(last_slash_pos + 1);
    }
    return path;
}

std::string format_milliseconds(int ms) {
    int hours = ms / 3600000;
    ms %= 3600000;
    int minutes = ms / 60000;
    ms %= 60000;
    int seconds = ms / 1000;
    int milliseconds = ms % 1000;

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hours << "_"
        << std::setfill('0') << std::setw(2) << minutes << "_"
        << std::setfill('0') << std::setw(2) << seconds << "_"
        << std::setfill('0') << std::setw(3) << milliseconds;
    return oss.str();
}


/**
 * @brief Extrae y guarda un único frame por cada bloque o cambio significativo de texto.
 * @param fileName Nombre del archivo de video.
 * @param net Red neuronal pre-cargada.
 */
void imgExtractor(const std::string &fileName, cv::dnn::Net &net) {
    using namespace std;
    using namespace cv;

    enum TextDetectionState { SEARCHING_FOR_TEXT, TEXT_IN_PROGRESS };

    string videoPath = VIDEOS_DIR + "/" + fileName;
    VideoCapture video(videoPath, VideoCaptureAPIs::CAP_FFMPEG);
    if (!video.isOpened()) {
        cerr << "Error al abrir el video: " << videoPath << endl;
        return;
    }
    
    string folderName = createFolderByFileName("RGBImages"); 

    double fps = video.get(CAP_PROP_FPS);
    int total_frames = static_cast<int>(video.get(CAP_PROP_FRAME_COUNT));
    cout << "Procesando: " << fileName << " (FPS: " << fps << ", Frames: " << total_frames << ")" << endl;
    
    // --- Variables de la máquina de estados ---
    TextDetectionState currentState = SEARCHING_FOR_TEXT;
    Mat textSceneFrame;
    Mat textSceneStartMask; // Máscara del inicio de la escena
    long long textSceneStartTimeMs = 0;
    int framesWithoutTextCounter = 0;
    long long lastProcessedFrameTimeMs = 0;

    for (int counter = 1; counter <= total_frames; ++counter) {
        Mat img;
        video >> img;
        if (img.empty()) break;

        if (counter % FRAME_SKIP_RATE == 0) {
            bool hasText = false;
            
            Mat resizedImg;
            resize(img, resizedImg, Size(DNN_INPUT_SIZE, DNN_INPUT_SIZE), INTER_AREA);
            Mat blob = dnn::blobFromImage(resizedImg, DNN_SCALE_FACTOR, Size(DNN_INPUT_SIZE, DNN_INPUT_SIZE), Scalar(), true, false);
            net.setInput(blob);
            vector<Mat> detections;
            net.forward(detections);

            Mat currentMask = detections[0].reshape(1, DNN_INPUT_SIZE) * 255;
            threshold(currentMask, currentMask, 127, 255, THRESH_BINARY);
            currentMask.convertTo(currentMask, CV_8U);
            
            vector<vector<Point> > contours;
            findContours(currentMask, contours, RETR_TREE, CHAIN_APPROX_SIMPLE);
            for (size_t i = 0; i < contours.size(); ++i) {
                if (contourArea(contours[i]) > AREA_THRESHOLD_MIN && contourArea(contours[i]) < AREA_THRESHOLD_MAX) {
                    hasText = true;
                    break;
                }
            }
            
            long long currentFrameTimeMs = static_cast<long long>(video.get(CAP_PROP_POS_MSEC));

            // --- Lógica de la máquina de estados ---
            if (currentState == SEARCHING_FOR_TEXT) {
                if (hasText) {
                    cout << "EVENTO: Inicio de texto en " << format_milliseconds(currentFrameTimeMs) << endl;
                    textSceneFrame = img.clone();
                    textSceneStartTimeMs = currentFrameTimeMs;
                    textSceneStartMask = currentMask.clone(); // Guardamos la máscara inicial
                    currentState = TEXT_IN_PROGRESS;
                    framesWithoutTextCounter = 0;
                }
            } else { // currentState == TEXT_IN_PROGRESS
                if (hasText) {
                    // --- Comparamos la máscara actual con la del inicio de la escena ---
                    Mat diff;
                    absdiff(currentMask, textSceneStartMask, diff);
                    int changedPixels = countNonZero(diff);

                    if (changedPixels > TEXT_CHANGE_PIXEL_THRESHOLD) {
                        cout << "EVENTO: Cambio significativo de texto detectado (" << changedPixels << " pixeles)." << endl;
                        // Guardamos la escena ANTERIOR
                        string imagePath = folderName + "/" + format_milliseconds(textSceneStartTimeMs) + "__" + format_milliseconds(lastProcessedFrameTimeMs) + ".jpeg";
                        if (imwrite(imagePath, textSceneFrame)) {
                             cout << "  > Imagen de escena anterior guardada: " << get_filename_from_path(imagePath) << endl;
                        }
                        
                        // Y empezamos una NUEVA escena con los datos actuales
                        cout << "  > Iniciando nueva escena de texto en " << format_milliseconds(currentFrameTimeMs) << endl;
                        textSceneFrame = img.clone();
                        textSceneStartTimeMs = currentFrameTimeMs;
                        textSceneStartMask = currentMask.clone();
                    }
                    
                    framesWithoutTextCounter = 0; // Reiniciamos contador de ausencia
                } else {
                    // El texto ha desaparecido, empezamos a contar para confirmar
                    framesWithoutTextCounter++;
                    if (framesWithoutTextCounter >= TEXT_ABSENCE_PERSISTENCE_THRESHOLD) {
                        cout << "EVENTO: Fin de texto confirmado." << endl;
                        string imagePath = folderName + "/" + format_milliseconds(textSceneStartTimeMs) + "__" + format_milliseconds(currentFrameTimeMs) + ".jpeg";
                        if (imwrite(imagePath, textSceneFrame)) {
                            cout << "  > Imagen guardada: " << get_filename_from_path(imagePath) << endl;
                        }
                        
                        currentState = SEARCHING_FOR_TEXT;
                    }
                }
            }
            lastProcessedFrameTimeMs = currentFrameTimeMs;
        }
    }
    
    if (currentState == TEXT_IN_PROGRESS) {
        cout << "EVENTO: Video terminó durante un bloque de texto. Guardando última escena." << endl;
        string imagePath = folderName + "/" + format_milliseconds(textSceneStartTimeMs) + "__" + format_milliseconds(lastProcessedFrameTimeMs) + ".jpeg";
        imwrite(imagePath, textSceneFrame);
        cout << "  > Imagen guardada: " << get_filename_from_path(imagePath) << endl;
    }

    video.release();
    destroyAllWindows();
    cout << "Extracción completada para el video: " << fileName << endl;
}


int main() {
    // El código de main no cambia
    struct stat st;
    if (stat(VIDEOS_DIR.c_str(), &st) == -1) { mkdir(VIDEOS_DIR.c_str()); }
    
    cout << "Cargando modelo DNN desde " << MODEL_PATH << "..." << endl;
    cv::dnn::Net net;
    try {
        net = cv::dnn::readNetFromONNX(MODEL_PATH);
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_OPENCL);
    } catch (const cv::Exception& e) {
        cerr << "Error al cargar el modelo DNN: " << e.what() << endl;
        return 1;
    }
    cout << "Modelo cargado exitosamente." << endl;

    list_files(VIDEOS_DIR, [&](const std::string& fileName) {
        imgExtractor(fileName, net);
    });
    
    cout << "Proceso completado." << endl;
    return 0;
}