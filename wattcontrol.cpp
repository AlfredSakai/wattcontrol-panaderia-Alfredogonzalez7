#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

#define HORAS_DIA 24

// Constantes globales del negocio (Panadería)
const double POTENCIA_CONTRATADA = 15.0; // kW máximos permitidos
const double PRECIO_CARO = 800.0;        // COP / kWh límite de alto costo
const double PRECIO_EMERGENCIA = 1500.0; // COP / kWh para activación de break
const double NIVEL_EMBALSE_ALERTA = 40.0; // % límite de alerta hídrica

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
        return true; // La carga crítica nunca se apaga por gestión económica
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

    int getHorasMinimas() const { return horasMinimasRequeridas; }
    int getHorasOperadas() const { return horasOperadasHoy; }
    void reiniciarHoras() { horasOperadasHoy = 0; }
    void incrementarHora() { horasOperadasHoy++; }

    // Condición compuesta con operadores lógicos (&&, ||, !)
    bool debeEncender(double precioKwh, double nivelEmbalse, bool horarioPunta) override {
        if (!(nivelEmbalse >= NIVEL_EMBALSE_ALERTA) || horarioPunta || (precioKwh > PRECIO_CARO)) {
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
    Medidor() : precioActualKwh(500.0), nivelEmbalses(55.0) {}
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
    double costoSinGestorDia;
    bool produccionCompleta;
public:
    Panaderia() : costoTotalDia(0), costoSinGestorDia(0), produccionCompleta(true) {
        cargas.push_back(new CargaCritica("Refrigerador", 1.5));
        cargas.push_back(new CargaAplazable("Horno Principal", 6.0, 4)); // Mínimo 4 horas
        cargas.push_back(new CargaAplazable("Amasadora Industrial", 3.0, 3)); // Mínimo 3 horas
    }

    ~Panaderia() {
        for (size_t i = 0; i < cargas.size(); i++) {
            delete cargas[i];
        }
    }

    vector<Carga*>& getCargas() { return cargas; }
    Medidor& getMedidor() { return medidor; }
    double getCostoTotal() const { return costoTotalDia; }
    double getCostoSinGestor() const { return costoSinGestorDia; }
    bool isProduccionCompleta() const { return produccionCompleta; }

    void simularDia() {
        costoTotalDia = 0;
        costoSinGestorDia = 0;
        produccionCompleta = true;
        srand(time(0));

        // Reiniciar contadores de horas operadas de las cargas aplazables
        for (size_t i = 0; i < cargas.size(); i++) {
            CargaAplazable* aplazable = dynamic_cast<CargaAplazable*>(cargas[i]);
            if (aplazable != nullptr) {
                aplazable->reiniciarHoras();
            }
        }

        cout << "\n=== SIMULACION DE 24 HORAS (PANADERIA - WATTCONTROL) ===\n";

        for (int hora = 0; hora < HORAS_DIA; hora++) {
            bool horaPunta = (hora >= 18 && hora <= 21);
            
            // Generar precio aleatorio según la franja real
            double precio = horaPunta ? (700.0 + (rand() % 400)) : (400.0 + (rand() % 250));
            double embalses = 30.0 + (rand() % 35); // Oscila entre 30% y 64%
            medidor.actualizarLecturas(precio, embalses);

            // Control de CONTINUE: 1 de cada 20 horas o simulación de falla de lectura de precio
            if (hora == 7 && (rand() % 2 == 0)) {
                cout << "[Hora 07:00] ¡ALERTA! Falla en la lectura del medidor de tarifas (continue).\n";
                cout << "  -> Operando unicamente con cargas criticas por seguridad.\n";
                for (size_t i = 0; i < cargas.size(); i++) {
                    bool encender = (dynamic_cast<CargaCritica*>(cargas[i]) != nullptr);
                    cargas[i]->setEstado(encender);
                    if (encender) {
                        costoTotalDia += cargas[i]->getPotencia() * precio;
                        costoSinGestorDia += cargas[i]->getPotencia() * precio;
                    }
                }
                continue; // Salta a la siguiente hora
            }

            // Control de BREAK: Emergencia de racionamiento (Precio > 1500 y Embalses < 40%)
            if (precio > PRECIO_EMERGENCIA && embalses < NIVEL_EMBALSE_ALERTA) {
                cout << "\n[Hora " << (hora < 10 ? "0" : "") << hora << ":00] ¡EMERGENCIA DE RACIONAMIENTO ELÉCTRICO! (break)\n";
                cout << "Precio crítico ($" << precio << ") y Embalses bajos (" << embalses << "%).\n";
                cout << "Se detiene la simulación de inmediato para proteger la red del negocio.\n";
                break;
            }

            cout << "[Hora " << (hora < 10 ? "0" : "") << hora << ":00] Precio: $" << precio 
                 << " | Embalses: " << embalses << "%" << (horaPunta ? " (PUNTA)" : "") << "\n";
            
            double potenciaActiva = 0;

            // Evaluación de decisiones de encendido mediante Polimorfismo
            for (size_t i = 0; i < cargas.size(); i++) {
                bool encender = cargas[i]->debeEncender(precio, embalses, horaPunta);
                cargas[i]->setEstado(encender);
                
                if (encender) {
                    potenciaActiva += cargas[i]->getPotencia();
                }
            }

            // REGLA 6: Validación de Potencia Contratada
            if (potenciaActiva > POTENCIA_CONTRATADA) {
                cout << "  [ALERTA SOBRECARGA] Potencia activa (" << potenciaActiva 
                     << " kW) supera el contrato (" << POTENCIA_CONTRATADA << " kW).\n";
                
                // Apagar la carga aplazable de mayor potencia (ej. Horno de 6.0 kW)
                for (int i = cargas.size() - 1; i >= 0; i--) {
                    if (cargas[i]->getEstado() && dynamic_cast<CargaAplazable*>(cargas[i]) != nullptr) {
                        cargas[i]->setEstado(false);
                        potenciaActiva -= cargas[i]->getPotencia();
                        cout << "  -> Apagado forzoso de proteccion: " << cargas[i]->getNombre() << "\n";
                        break;
                    }
                }
            }

            // Acumular costos y horas operadas
            for (size_t i = 0; i < cargas.size(); i++) {
                if (cargas[i]->getEstado()) {
                    costoTotalDia += cargas[i]->getPotencia() * precio;
                    CargaAplazable* aplazable = dynamic_cast<CargaAplazable*>(cargas[i]);
                    if (aplazable != nullptr) {
                        aplazable->incrementarHora();
                    }
                }
                // Costo sin gestor: todo encendido todo el día (asumiendo operación base)
                costoSinGestorDia += cargas[i]->getPotencia() * precio;

                cout << "  - " << cargas[i]->getNombre() << " -> " 
                     << (cargas[i]->getEstado() ? "ENCENDIDA" : "APLAZADA/APAGADA") << "\n";
            }
        }

        // Verificación de horas mínimas de producción
        for (size_t i = 0; i < cargas.size(); i++) {
            CargaAplazable* aplazable = dynamic_cast<CargaAplazable*>(cargas[i]);
            if (aplazable != nullptr) {
                if (aplazable->getHorasOperadas() < aplazable->getHorasMinimas()) {
                    produccionCompleta = false;
                }
            }
        }

        cout << "\nSimulación completada con éxito.\n";
    }

    void mostrarResumen() {
        cout << "\n=========================================\n";
        cout << "       RESUMEN DE LA ULTIMA SIMULACION     \n";
        cout << "=========================================\n";
        cout << "Equipos gestionados en la Panaderia:\n";
        for (size_t i = 0; i < cargas.size(); i++) {
            CargaAplazable* aplazable = dynamic_cast<CargaAplazable*>(cargas[i]);
            cout << " - " << cargas[i]->getNombre() << " (" << cargas[i]->getPotencia() << " kW)";
            if (aplazable != nullptr) {
                cout << " | Horas operadas: " << aplazable->getHorasOperadas() 
                     << "h (Minimo requerido: " << aplazable->getHorasMinimas() << "h)";
            } else {
                cout << " | Operacion continua critica";
            }
            cout << "\n";
        }

        cout << "\n-----------------------------------------\n";
        cout << " Costo total CON gestor WattControl: $" << costoTotalDia << " COP\n";
        cout << " Costo total SIN gestor energético:  $" << costoSinGestorDia << " COP\n";
        
        if (costoSinGestorDia > 0) {
            double ahorroPesos = costoSinGestorDia - costoTotalDia;
            double ahorroPorcentaje = (ahorroPesos / costoSinGestorDia) * 100.0;
            cout << " AHORRO OBTENIDO:                   $" << ahorroPesos << " COP (" << ahorroPorcentaje << "%)\n";
        }

        if (!produccionCompleta) {
            cout << "\n [ALERTA] ATENCION: ¡Producción incompleta por restricciones energéticas severas!\n";
        } else {
            cout << "\n [ESTADO] Produccion completada cumpliendo metas de calidad y horneado.\n";
        }
        cout << "=========================================\n";
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
        cout << "4. Mostrar resumen de la ultima simulacion\n";
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
                double precio = panaderia.getMedidor().getPrecio();
                double embalses = panaderia.getMedidor().getEmbalses();
                bool punta = (precio > 800.0);
                
                for (size_t i = 0; i < listaCargas.size(); i++) {
                    bool estado = listaCargas[i]->debeEncender(precio, embalses, punta);
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
