#include "wifi_ap.h"
#include "web_pages.h"

#ifndef UNIT_TEST
#include <EEPROM.h>     // must precede params.h (via web_api.h)
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include "SD.h"
#include "web_api.h"
#include "controller.h"
#include "drive_config.h"

static WebServer         sServer(80);
static DNSServer         sDNS;
static AmidalaController* sCtrl = nullptr;

// ---------------------------------------------------------------------------
// SD card config write-back
// ---------------------------------------------------------------------------

static bool updateConfigFile(const char* key, const char* value) {
    String path   = "/config.txt";
    String prefix = String(key) + "=";
    String newLine = prefix + String(value);

    File f = SD.open(path, "r");
    if (!f) {
        // No config.txt yet — create a minimal one
        File wf = SD.open(path, "w");
        if (!wf) return false;
        wf.println("#START");
        wf.println(newLine);
        wf.println("#END");
        wf.close();
        return true;
    }

    String out;
    out.reserve(8192);
    bool found = false;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        // Trim trailing \r so the comparison works on Windows-style line endings
        if (line.endsWith("\r")) line.remove(line.length() - 1);

        // Replace a matching non-commented key= line
        if (!line.startsWith("#") && !line.startsWith("//") && line.startsWith(prefix)) {
            out += newLine + "\n";
            found = true;
        } else {
            out += line + "\n";
        }
    }
    f.close();

    if (!found) {
        // Insert before #END if present, otherwise append
        int endIdx = out.lastIndexOf("#END");
        if (endIdx >= 0) {
            out = out.substring(0, endIdx) + newLine + "\n" + out.substring(endIdx);
        } else {
            out += newLine + "\n";
        }
    }

    SD.remove(path);
    File wf = SD.open(path, "w");
    if (!wf) return false;
    wf.print(out);
    wf.close();
    return true;
}

// ---------------------------------------------------------------------------
// Serial-string config file helpers
// ---------------------------------------------------------------------------

static bool rewriteSerialStrings() {
    String path = "/config.txt";
    File f = SD.open(path, "r");
    String out;
    out.reserve(8192);

    if (f) {
        while (f.available()) {
            String line = f.readStringUntil('\n');
            if (line.endsWith("\r")) line.remove(line.length() - 1);
            if (!line.startsWith("sstr="))
                out += line + "\n";
        }
        f.close();
    } else {
        out = "#START\n#END\n";
    }

    // Build new sstr= lines
    String sstrs;
    for (uint8_t i = 0; i < sCtrl->params.serialcount; i++) {
        sstrs += "sstr=";
        sstrs += sCtrl->params.Str[i].name;
        sstrs += "|";
        sstrs += sCtrl->params.Str[i].str;
        sstrs += "\n";
    }

    // Insert before #END if present, otherwise append
    int endIdx = out.lastIndexOf("#END");
    if (endIdx >= 0)
        out = out.substring(0, endIdx) + sstrs + out.substring(endIdx);
    else
        out += sstrs;

    SD.remove(path);
    File wf = SD.open(path, "w");
    if (!wf) return false;
    wf.print(out);
    wf.close();
    return true;
}

// ---------------------------------------------------------------------------
// REST API handlers
// ---------------------------------------------------------------------------

static void handleApiInfo() {
    if (!sCtrl) { sServer.send(500, "application/json", "{}"); return; }

#if DRIVE_SYSTEM == DRIVE_SYSTEM_SABER
    const char* drive = "sabertooth";
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_PWM
    const char* drive = "pwm";
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_SERIAL
    const char* drive = "roboteq-serial";
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_PWM
    const char* drive = "roboteq-pwm";
#elif DRIVE_SYSTEM == DRIVE_SYSTEM_ROBOTEQ_PWM_SERIAL
    const char* drive = "roboteq-pwm-serial";
#else
    const char* drive = nullptr;
#endif
#if DOME_DRIVE == DOME_DRIVE_ROBOCLAW
    const char* dome = "roboclaw";
#elif DOME_DRIVE == DOME_DRIVE_PWM
    const char* dome = "pwm";
#elif DOME_DRIVE == DOME_DRIVE_SABER
    const char* dome = "sabertooth";
#else
    const char* dome = nullptr;
#endif
#ifdef VMUSIC_SERIAL
    const char* audio = "vmusic";
#else
    const char* audio = "hcr";
#endif

    sServer.send(200, "application/json",
        buildInfoJson(drive, dome, audio,
                      sCtrl->params.wifiSSID,
                      WiFi.softAPIP().toString().c_str()));
}

static void handleApiConfigGet() {
    if (!sCtrl) { sServer.send(500, "application/json", "{}"); return; }
    sServer.send(200, "application/json",
        buildFullConfigJson(sCtrl->params));
}

static void setSerialStringFields(SerialString& s, const char* val) {
    const char* pipe = strchr(val, '|');
    if (pipe) {
        size_t nlen = min((size_t)(pipe - val), sizeof(s.name) - 1);
        memcpy(s.name, val, nlen);
        s.name[nlen] = '\0';
        strncpy(s.str, pipe + 1, sizeof(s.str) - 1);
        s.str[sizeof(s.str) - 1] = '\0';
    } else {
        s.name[0] = '\0';
        strncpy(s.str, val, sizeof(s.str) - 1);
        s.str[sizeof(s.str) - 1] = '\0';
    }
}

static void handleApiConfigPost() {
    if (!sCtrl) { sServer.send(500, "text/plain", "no controller"); return; }

    String key   = sServer.arg("key");
    String value = sServer.arg("value");
    if (key.isEmpty()) { sServer.send(400, "text/plain", "missing key"); return; }

    // sstr_del_N — delete serial string at index N, shift remainder down
    if (key.startsWith("sstr_del_")) {
        int idx = key.substring(9).toInt();
        if (idx < 0 || idx >= (int)sCtrl->params.serialcount) {
            sServer.send(400, "text/plain", "index out of range"); return;
        }
        for (int j = idx; j < (int)sCtrl->params.serialcount - 1; j++)
            sCtrl->params.Str[j] = sCtrl->params.Str[j + 1];
        memset(&sCtrl->params.Str[sCtrl->params.serialcount - 1], 0, sizeof(SerialString));
        sCtrl->params.serialcount--;
        bool ok = rewriteSerialStrings();
        sServer.send(ok ? 200 : 207, "text/plain", ok ? "OK" : "deleted but SD write failed");
        return;
    }

    // sstr_N — update (or append when N == serialcount) serial string at index N
    if (key.startsWith("sstr_")) {
        int idx = key.substring(5).toInt();
        if (idx < 0 || idx > (int)sCtrl->params.serialcount ||
            idx >= (int)sCtrl->params.getSerialStringCount()) {
            sServer.send(400, "text/plain", "index out of range"); return;
        }
        setSerialStringFields(sCtrl->params.Str[idx], value.c_str());
        if (idx == (int)sCtrl->params.serialcount)
            sCtrl->params.serialcount++;
        bool ok = rewriteSerialStrings();
        sServer.send(ok ? 200 : 207, "text/plain", ok ? "OK" : "applied but SD write failed");
        return;
    }

    // Generic key=value — delegate to processConfig
    String cmd = key + "=" + value;
    if (!sCtrl->fConfig.processConfig(cmd.c_str())) {
        sServer.send(400, "text/plain", "unknown setting: " + key);
        return;
    }
    if (!updateConfigFile(key.c_str(), value.c_str())) {
        sServer.send(207, "text/plain", "applied but SD write failed");
        return;
    }
    sServer.send(200, "text/plain", "OK");
}

// ---------------------------------------------------------------------------
// Page handlers
// ---------------------------------------------------------------------------

static void handleHome() {
    sServer.send(200, "text/html", WEB_PAGE_HOME);
}

static void handleConfigGeneral()       { sServer.send(200, "text/html", WEB_PAGE_GENERAL);        }
static void handleConfigWifi()          { sServer.send(200, "text/html", WEB_PAGE_WIFI);            }
static void handleConfigXbee()          { sServer.send(200, "text/html", WEB_PAGE_XBEE);            }
static void handleConfigAudio()         { sServer.send(200, "text/html", WEB_PAGE_AUDIO);           }
static void handleConfigRcRadio()       { sServer.send(200, "text/html", WEB_PAGE_RC_RADIO);        }
static void handleConfigDome()          { sServer.send(200, "text/html", WEB_PAGE_DOME);            }
static void handleConfigSerialStrings() { sServer.send(200, "text/html", WEB_PAGE_SERIAL_STRINGS);  }
static void handleComingSoon()          { sServer.send(200, "text/html", WEB_PAGE_COMING_SOON);     }

// ---------------------------------------------------------------------------
// AmidalaWiFiAP
// ---------------------------------------------------------------------------

void AmidalaWiFiAP::begin(const char* ssid, const char* password, AmidalaController* ctrl) {
    sCtrl = ctrl;

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    IPAddress ip = WiFi.softAPIP();
    Serial.print(F("[WiFi] AP \""));
    Serial.print(ssid);
    Serial.print(F("\" @ "));
    Serial.println(ip);

    // Wildcard DNS: all hostnames → AP IP (captive-portal style)
    sDNS.start(53, "*", ip);

    // mDNS: advertise <ssid>.local
    if (MDNS.begin(ssid)) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("[WiFi] mDNS: http://%s.local\n", ssid);
    }

    // Pages
    sServer.on("/",                      HTTP_GET, handleHome);
    sServer.on("/index.html",            HTTP_GET, handleHome);
    sServer.on("/config/general",        HTTP_GET, handleConfigGeneral);
    sServer.on("/config/wifi",           HTTP_GET, handleConfigWifi);
    sServer.on("/config/xbee",           HTTP_GET, handleConfigXbee);
    sServer.on("/config/audio",          HTTP_GET, handleConfigAudio);
    sServer.on("/config/rc-radio",       HTTP_GET, handleConfigRcRadio);
    sServer.on("/config/dome",           HTTP_GET, handleConfigDome);
    // Remaining config pages (stubs)
    sServer.on("/config/buttons",        HTTP_GET, handleComingSoon);
    sServer.on("/config/servos",         HTTP_GET, handleComingSoon);
    sServer.on("/config/serial-strings", HTTP_GET, handleConfigSerialStrings);
    sServer.on("/sequences",            HTTP_GET, handleComingSoon);
    sServer.on("/monitor",              HTTP_GET, handleComingSoon);
    sServer.on("/update",               HTTP_GET, handleComingSoon);

    // REST API
    sServer.on("/api/info",   HTTP_GET,  handleApiInfo);
    sServer.on("/api/config", HTTP_GET,  handleApiConfigGet);
    sServer.on("/api/config", HTTP_POST, handleApiConfigPost);

    // Catch-all: any other URL redirects home (supports captive-portal flow)
    sServer.onNotFound(handleHome);

    sServer.begin();
    Serial.println(F("[WiFi] HTTP server started"));
}

void AmidalaWiFiAP::handle() {
    sDNS.processNextRequest();
    sServer.handleClient();
}

#else  // UNIT_TEST

void AmidalaWiFiAP::begin(const char*, const char*, AmidalaController*) {}
void AmidalaWiFiAP::handle() {}

#endif
