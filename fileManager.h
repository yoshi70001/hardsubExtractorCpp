#ifndef FILEMANAGER_H_INCLUDED
#define FILEMANAGER_H_INCLUDED
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
using namespace std;
void list_files(const string &path, function<void(string &)> callback)
{
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr)
    {
        cerr << "No se puede abrir el directorio: " << std::strerror(errno) << std::endl;
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        string file_name = entry->d_name;
        if (file_name.length() >= 4)
        {
            string extension = file_name.substr(file_name.length() - 4);
            if (extension == ".mp4" || extension == ".mkv" || extension == ".avi")
            {
                cout << file_name << endl;

                callback(file_name);
            }
        }
    }

    closedir(dir);
}
bool existeDirectorio(const string &path)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
    {
        return false; // No se puede acceder al directorio
    }
    else if (info.st_mode & S_IFDIR)
    {
        return true; // Es un directorio
    }
    else
    {
        return false; // No es un directorio
    }
}
// Función que reemplaza caracteres especiales y espacios por '_'
void reemplazar_caracteres(char *str)
{
    while (*str != '\0')
    {
        // Si no es un carácter alfanumérico, lo reemplazamos por '_'
        if (!isalnum((unsigned char)*str))
        {
            *str = '_';
        }
        str++;
    }
}
string createFolderByFileName(string filenameVideo)
{
    // Extraer el nombre del archivo sin la extensión
    std::string foldername = filenameVideo.substr(0, filenameVideo.find_last_of('.'));
    char *buffer = new char[foldername.size() + 1]; // +1 para el carácter nulo
    std::strcpy(buffer, foldername.c_str());
    reemplazar_caracteres(buffer);
    // Crear la carpeta con el nombre extraído
    if (mkdir(buffer) == 0)
    {
        std::cout << "Carpeta creada: " << foldername << std::endl;
    }
    else
    {
        // if (errno == 17)
        // {
        //     rmdir(foldername.c_str());
        //     createFolderByFileName(filenameVideo);
        // }
        std::cerr << "Error al crear la carpeta: " << strerror(errno) << std::endl;
    }
    // delete buffer;
    return buffer;
}

#endif // FILEMANAGER_H_INCLUDED
