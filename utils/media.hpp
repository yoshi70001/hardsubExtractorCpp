#include <iostream>
#include <vector>
#include <cmath>  // Para std::abs()
#include <numeric> // Para std::accumulate()

class DetectorCambioBrusco {
private:
    std::vector<int> cambios_recientes;
    size_t ventana;
    double factor_umbral; // Factor para multiplicar la media

public:
    DetectorCambioBrusco(size_t ventana_size, double factor)
        : ventana(ventana_size), factor_umbral(factor) {}

    bool detectar(int valor_actual, int &valor_anterior) {
        int cambio = std::abs(valor_actual - valor_anterior);

        // Agregar el cambio a la lista
        if (cambios_recientes.size() >= ventana) {
            cambios_recientes.erase(cambios_recientes.begin()); // Eliminar el más antiguo
        }
        cambios_recientes.push_back(cambio);

        // Calcular la media móvil
        double media_movil = cambios_recientes.empty() ? 0 : 
                             std::accumulate(cambios_recientes.begin(), cambios_recientes.end(), 0.0) / cambios_recientes.size();

        // Definir umbral dinámico
        double umbral = media_movil * factor_umbral;

        // Detectar cambio brusco
        if (cambio > umbral) {
            // std::cout << "¡Cambio brusco detectado! Valor anterior: " << valor_anterior
            //           << ", Valor actual: " << valor_actual
            //           << " (Cambio: " << cambio << ", Umbral: " << umbral << ")\n";
            return true; // Cambio brusco detectado
        }
        
        valor_anterior = valor_actual; // Actualizar para la siguiente iteración
        return false; // No se detectó cambio brusco
    }
};