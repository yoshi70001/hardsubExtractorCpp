#include <iostream>
#include <fstream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>  // Asegúrate de tener la biblioteca nlohmann/json instalada

using json = nlohmann::json;

// Función para obtener el token de acceso (debes implementarla correctamente)
std::string getAccessToken() {
    return "ya29.a0AeXRPp5LL_vCy9yIp0EHMDj82hrhs2g3CozkD3_bIciDwM_QHKuuswFrBzyht9Fmdv1-N-Ky4FdORbfKf_U530_GmO7z7W__PHwOHIoV_XJ_M3kwrzKohecWKl9GuwwqLyedz70WJT2_-XWKfBA_zgq2aJdnu-wK0BwwdPk0aCgYKAaISARMSFQHGX2Miqt6MYXddtKcqbicNP_WEaA0175";
}

// Función de callback para cURL
size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

// 1️⃣ Subir la imagen a Google Drive con OCR habilitado
std::string uploadImageForOCR(const std::string& imagePath) {
    std::string accessToken = getAccessToken();
    std::string uploadUrl = "https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart";

    CURL* curl;
    CURLcode res;
    struct curl_slist* headers = NULL;

    std::ifstream file(imagePath, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string fileData = buffer.str();

    json metadata = {
        {"name", "OCR_Document"},
        {"mimeType", "application/vnd.google-apps.document"}
    };
    std::string metadataStr = metadata.dump();

    std::string requestBody = "--boundary123\r\n"
                              "Content-Type: application/json; charset=UTF-8\r\n\r\n" +
                              metadataStr + "\r\n--boundary123\r\n"
                              "Content-Type: image/jpeg\r\n\r\n" + fileData +
                              "\r\n--boundary123--";
    curl = curl_easy_init();
    std::string response;

    if (curl) {
        headers = curl_slist_append(headers, "Content-Type: multipart/related; boundary=boundary123");
        headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
         // Especificar el archivo de certificados
        curl_easy_setopt(curl, CURLOPT_CAINFO, "C:\\Users\\surke\\Desktop\\hardsubExtractor\\cacert.pem");
        curl_easy_setopt(curl, CURLOPT_URL, uploadUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, requestBody.size());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Error en la subida: " << curl_easy_strerror(res) << std::endl;
            return "";
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    json jsonResponse = json::parse(response);
    return jsonResponse["id"].get<std::string>();
}

// 2️⃣ Descargar el contenido del Google Docs como texto
void downloadTextFromGoogleDoc(const std::string& docId, const std::string& outputFilePath) {
    std::string accessToken = getAccessToken();
    std::string exportUrl = "https://www.googleapis.com/drive/v3/files/" + docId + "/export?mimeType=text/plain";

    CURL* curl;
    CURLcode res;
    struct curl_slist* headers = NULL;
    std::string response;

    curl = curl_easy_init();
    if (curl) {
        headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
         // Especificar el archivo de certificados
        curl_easy_setopt(curl, CURLOPT_CAINFO, "C:\\Users\\surke\\Desktop\\hardsubExtractor\\cacert.pem");
        curl_easy_setopt(curl, CURLOPT_URL, exportUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Error al descargar el texto: " << curl_easy_strerror(res) << std::endl;
        } else {
            std::ofstream outFile(outputFilePath);
            outFile << response;
            outFile.close();
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
}

// int main() {
//     std::string imagePath = "./EnjoKouhai_2_gFA/00_00_18_593__00_00_20_236.jpeg";  // Cambia por tu imagen
//     std::string outputFilePath = "./EnjoKouhai_2_gFA/00_00_18_593__00_00_20_236.txt";

//     std::string docId = uploadImageForOCR(imagePath);
//     if (!docId.empty()) {
//         std::cout << "Documento creado con ID: " << docId << std::endl;
//         downloadTextFromGoogleDoc(docId, outputFilePath);
//     } else {
//         std::cerr << "Error al subir la imagen." << std::endl;
//     }

//     return 0;
// }
std::string getTextByImage(std::string imagePath){
    std::string outputFilePath = imagePath.substr(0, imagePath.size() - 4) + ".txt";

    std::string docId = uploadImageForOCR(imagePath);
    if (!docId.empty()) {
        // std::cout << "Documento creado con ID: " << docId << std::endl;
        downloadTextFromGoogleDoc(docId, outputFilePath);
        return outputFilePath;
    } else {
        std::cerr << "Error al subir la imagen." << std::endl;
        return "Error";
    }
}