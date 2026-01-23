// example_wifi_config.h
// Primer konfiguracijske datoteke za WiFi - KOPIRAJ v wifi_config.h in prilagodi!

#ifndef WIFI_CONFIG_EXAMPLE_H
#define WIFI_CONFIG_EXAMPLE_H

#include <WiFi.h>

// Primer seznama Wi-Fi omrežij - zamenjaj z lastnimi podatki
const char* ssidList[] = {
    "MojeOmrezje1",
    "MojeOmrezje2"
};
const char* passwordList[] = {
    "geslo123456",
    "drugoGeslo789"
};
const int numNetworks = 2;

// Primer statične IP konfiguracije - prilagodi glede na svoje omrežje
IPAddress localIP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);          // Google DNS

// Primer AP (Access Point) nastavitev - za fallback, če ni povezave
const char* apSSID = "VentilacijaAP";
const char* apPassword = "vent12345";

#endif // WIFI_CONFIG_EXAMPLE_H