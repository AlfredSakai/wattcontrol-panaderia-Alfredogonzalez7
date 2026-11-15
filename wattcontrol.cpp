#include <iostream>
#include <string>
#include <vector>

using namespace std;

// Clase Base Abstracta Carga
class Carga {
protected:
    string nombre;
    double potenciaKw;
    bool estadoActual;
public:
    Carga(string nom, double pot) : nombre(nom), potenciaKw(pot), estadoActual(false) {}
    virtual ~Carga() {}

    string getNombre() const { return nombre; }
    double getPotencia() const { return potenciaKw; }
    bool getEstado() const { return estadoActual; }
    void setEstado(bool est) { estadoActual = est; }

    virtual bool debeEncender(double precioKwh, double nivelEmbalse, bool horarioPunta) = 0;
};

// Carga Crítica (Refrigerador)
class CargaCritica : public Carga {
public:
    CargaCritica(string nom, double pot) : Carga(nom, pot) {}
    bool debeEncender(double precioKwh, double nivelEmbalse, bool horarioPunta) override {
        return true;
    }
};

// Carga Aplazable (Horno, Amasadora)
class CargaAplazable : public Carga {
private:
    int horasMinimasRequeridas;
    int horasOperadasHoy;
public:
    CargaAplazable(string nom, double pot, int horasMin) 
        : Carga(nom, pot), horasMinimasRequeridas(horasMin), horasOperadasHoy(0) {}

    bool debeEncender(double precioKwh, double nivelEmbalse, bool horarioPunta) override {
        if (horarioPunta || precioKwh > 800.0) {
            return false;
        }
        return true;
    }
};

// Clase Medidor
class Medidor {
private:
    double precioActualKwh;
    double nivelEmbalses;
public:
    Medidor() : precioActualKwh(500.0), nivelEmbalses(60.0) {}
    void actualizarLecturas(double precio, double embalses) {
        precioActualKwh = precio;
        nivelEmbalses = embalses;
    }
    double getPrecio() const { return precioActualKwh; }
    double getEmbalses() const { return nivelEmbalses; }
};

// Clase Negocio / Panadería
class Panaderia {
private:
    vector<Carga*> cargas;
    Medidor medidor;
    double costoTotalDia;
public:
    Panaderia() : costoTotalDia(0) {
        cargas.push_back(new CargaCritica("Refrigerador", 1.5));
        cargas.push_back(new CargaAplazable("Horno", 6.0, 4));
        cargas.push_back(new CargaAplazable("Amasadora", 3.0, 3));
    }

    ~Panaderia() {
        for (size_t i = 0; i < cargas.size(); i++) {
            delete cargas[i];
        }
    }

    vector<Carga*>& getCargas() { return cargas; }
    Medidor& getMedidor() { return medidor; }

    void simularDia() {
        costoTotalDia = 0;
        cout << "\n=== SIMULACION DE 24 HORAS (PANADERIA) ===\n";
        for (int hora = 0; hora < 24; hora++) {
            bool horaPunta = (hora >= 18 && hora <= 21);
            double precio = horaPunta ? 950.0 : 450.0;
            medidor.actualizarLecturas(precio, 45.0);

            cout << "[Hora " << (hora < 10 ? "0" : "") << hora << ":00] Precio: $" << precio << (horaPunta ? " (PUNTA)" : "") << "\n";
            
            for (size_t i = 0; i < cargas.size(); i++) {
                bool encender = cargas[i]->debeEncender(precio, medidor.getEmbalses(), horaPunta);
                cargas[i]->setEstado(encender);
                if (encender) {
                    costoTotalDia += cargas[i]->getPotencia() * precio;
                }
                cout << "  - " << cargas[i]->getNombre() << " -> " << (encender ? "ENCENDIDA" : "APLAZADA/APAGADA") << "\n";
            }
        }
        cout << "\nSimulacion completada. Costo total estimado del dia: $" << costoTotalDia << "\n";
    }

    void mostrarResumen() {
        cout << "\n=== RESUMEN DE LA PANADERIA ===\n";
        cout << "Total de equipos gestionados: " << cargas.size() << "\n";
        for (size_t i = 0; i < cargas.size(); i++) {
            cout << "- " << cargas[i]->getNombre() << " (" << cargas[i]->getPotencia() << " kW)\n";
        }
    }
};

// Función Principal
int main() {
    Panaderia panaderia;
    int opcion = 0;
    double precioManual = 500.0;
    double embalseManual = 50.0;

    do {
        cout << "\n=========================================\n";
        cout << "       WATTCONTROL - PANADERIA           \n";
        cout << "=========================================\n";
        cout << "1. Registrar lectura manual (Medidor)\n";
        cout << "2. Evaluar estado actual de cargas\n";
        cout << "3. Simular ciclo completo de 24 horas\n";
        cout << "4. Mostrar resumen del negocio\n";
        cout << "5. Salir\n";
        cout << "Seleccione una opcion: ";
        cin >> opcion;

        switch (opcion) {
            case 1: {
                do {
                    cout << "Ingrese el precio de la energia (> 0): ";
                    cin >> precioManual;
                } while (precioManual <= 0);

                do {
                    cout << "Ingrese el nivel de los embalses (0 a 100 %): ";
                    cin >> embalseManual;
                } while (embalseManual < 0 || embalseManual > 100);

                panaderia.getMedidor().actualizarLecturas(precioManual, embalseManual);
                cout << "Lectura manual registrada con exito.\n";
                break;
            }
            case 2: {
                cout << "\n--- EVALUACION PUNTUAL DE CARGAS (PANADERIA) ---\n";
                vector<Carga*>& listaCargas = panaderia.getCargas();
                bool punta = (panaderia.getMedidor().getPrecio() > 800.0);
                for (size_t i = 0; i < listaCargas.size(); i++) {
                    bool estado = listaCargas[i]->debeEncender(panaderia.getMedidor().getPrecio(), panaderia.getMedidor().getEmbalses(), punta);
                    cout << "- " << listaCargas[i]->getNombre() << " -> Estado sugerido: " 
                         << (estado ? "ENCENDIDA" : "APLAZADA/APAGADA") << "\n";
                }
                break;
            }
            case 3:
                panaderia.simularDia();
                break;
            case 4:
                panaderia.mostrarResumen();
                break;
            case 5:
                cout << "Saliendo de WattControl. ¡Hasta luego!\n";
                break;
            default:
                cout << "Opcion invalida. Intente de nuevo.\n";
        }
    } while (opcion != 5);

    cout << "\nPrograma finalizado correctamente.\n";
    return 0;
}