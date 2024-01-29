#include <iostream>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <unistd.h>
#include <ctime>
#include <iomanip>
#include <curl/curl.h>

// Direcciones y comandos para el TCS34725
#define TCS34725_ADDRESS 0x29 //Sensor address
#define TCS34725_COMMAND_BIT 0x80
#define TCS34725_ENABLE 0x00
#define TCS34725_ENABLE_AIEN 0x10
#define TCS34725_ENABLE_WEN 0x08
#define TCS34725_ENABLE_AEN 0x02
#define TCS34725_ENABLE_PON 0x01
#define TCS34725_ATIME 0x01 //Integration time
#define TCS34725_RDATAL 0x16
#define TCS34725_GDATAL 0x18
#define TCS34725_BDATAL 0x1A
#define TCS34725_CDATAL 0x14

int fd;  // File descriptor para la interfaz I2C

void writeRegister(int reg, int value) {
    wiringPiI2CWriteReg8(fd, TCS34725_COMMAND_BIT | reg, value);
}

int read16(int reg) {
    int low = wiringPiI2CReadReg8(fd, TCS34725_COMMAND_BIT | reg);
    int high = wiringPiI2CReadReg8(fd, TCS34725_COMMAND_BIT | reg + 1);
    return (high << 8) | low;
}

void initializeSensor() {
    std::cout << "Initializing sensor: TCS34725" << std::endl;
    writeRegister(TCS34725_ENABLE, TCS34725_ENABLE_PON);
    sleep(1); // Espera de 1 segundo para el encendido del sensor
    writeRegister(TCS34725_ENABLE, TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN);
    writeRegister(TCS34725_ATIME, 0xEB);  // Configurar tiempo de integración para RGBC
}

int readRed() {
    return read16(TCS34725_RDATAL);
}

int readGreen() {
    return read16(TCS34725_GDATAL);
}

int readBlue() {
    return read16(TCS34725_BDATAL);
}

void sendData(std::string jsonData){
    CURL *curl;
    CURLcode res;

    // Initialize a CURL session
    curl = curl_easy_init();

    if (curl) {
        // Set the URL for the POST request
        curl_easy_setopt(curl, CURLOPT_URL, "https://srv-iot.diatel.upm.es/api/v1/cAsCPUCDGBxl3aVeso4S/telemetry");

        // Specify that we are sending a POST request
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // Provide the data for the POST request
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());

        // Set the size of the POST fields
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, jsonData.size());

        // Set the Content-Type header
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Perform the request, res will get the return code
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            std::cerr << "  curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            std::cout << "  curl_easy_perform() status: " << curl_easy_strerror(res) << std::endl;
        }

        // Cleanup
        curl_slist_free_all(headers); // Free the header list
        curl_easy_cleanup(curl);      // Always cleanup
    } else {
        std::cerr << "Error initializing CURL" << std::endl;
    }
}

int main() {
    fd = wiringPiI2CSetup(TCS34725_ADDRESS);
    if (fd == -1) {
        std::cerr << "Error al iniciar I2C" << std::endl;
        return 1;
    }

    initializeSensor();

    int count = 0;
    long totalRed = 0, totalGreen = 0, totalBlue = 0;

    while (true) {
        int red = readRed();
        int green = readGreen();
        int blue = readBlue();

        totalRed += red;
        totalGreen += green;
        totalBlue += blue;
        count++;

        std::ostringstream jsonDataStream;
        jsonDataStream << "{\"red\":" << red << ",\"green\":" << green << ",\"blue\":" << blue << ",\"red_avg\":" << (totalRed/count) << ",\"green_avg\":" << (totalGreen/count) << ",\"blue_avg\":" << (totalBlue/count) << "}";
        std::string jsonData = jsonDataStream.str();

        std::time_t now = std::time(nullptr);

        //Console Logs
        std::cout << "[" << now << "] --> Payload: " << jsonData << std::endl;

        sendData(jsonData);

        std::cout << std::endl;

        sleep(1);
    }

    return 0;
}
