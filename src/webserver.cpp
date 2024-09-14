#include "webserver.hpp"
#include "filesystem.hpp"
#include "leds.hpp"
#include "macros.hpp"

AsyncWebServer webServer(80); // webserver at Port 80
IPAddress localIP;
IPAddress localGateway;
IPAddress localSubnet;
DNSServer dnsServer;

/* Endpoint params main webpage */
const char *PARAM_R = "r";
const char *PARAM_G = "g";
const char *PARAM_B = "b";
const char *PARAM_MODE = "select";

/* Endpoint params wifi manager */
const char *PARAM_INPUT_0 = "ssid";
const char *PARAM_INPUT_1 = "pass";
const char *PARAM_INPUT_2 = "ip";
const char *PARAM_INPUT_3 = "gateway";
const char *PARAM_INPUT_4 = "subnet";
const String MANAGER = "http://8.8.8.8";

/* Access Point settings */
const String AP_SSID = "LED-STRIP";
const String AP_PW = "eisdiele";

/* Time settings*/
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
const char *url = "https://api.sunrise-sunset.org/\
json?lat=51.0801600&lng=9.2703400";

/* external vars */
extern String creds[5];
extern uint8_t r;
extern uint8_t g;
extern uint8_t b;

bool wmanager = false;
uint8_t mode = _OFF;
bool reset = 0;

// Function to convert "HH:MM:SS AM/PM" to struct tm
bool convertToTM(const char *timeStr, struct tm *timeinfo)
{
    int hour, minute, second;
    char ampm[3];

    // Parse the time string
    int parsed = sscanf(timeStr, "%d:%d:%d %2s", &hour, &minute, &second, ampm);

    if (parsed != 4)
    {
        return false; // Parsing error
    }

    // Convert to 24-hour format
    if (strcmp(ampm, "PM") == 0 && hour != 12)
    {
        hour += 12;
    }
    else if (strcmp(ampm, "AM") == 0 && hour == 12)
    {
        hour = 0; // Midnight case
    }

    // Fill the tm struct
    timeinfo->tm_hour = hour;
    timeinfo->tm_min = minute;
    timeinfo->tm_sec = second;
    timeinfo->tm_isdst = -1; // Daylight saving time (not used here)

    return true;
}

// Convert a struct tm (containing time) to seconds since midnight
int timeToSecondsSinceMidnight(struct tm *timeinfo)
{
    return (timeinfo->tm_hour * 3600) + (timeinfo->tm_min * 60) + timeinfo->tm_sec;
}

bool getSunsetSunrise(struct tm *sunrise_tm, struct tm *sunset_tm)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        WiFiClientSecure client;
        client.setInsecure(); // skip cert validation
        http.begin(client, url);
        int httpCode = http.GET();
        // Check for successful response
        if (httpCode > 0)
        {
            String payload = http.getString(); // Get the response payload
            // Serial.println("HTTP Response: " + payload);

            // Parse the JSON payload
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (error)
            {
                Serial.print("JSON parsing failed: ");
                Serial.println(error.c_str());
                return false;
            }

            // Extract sunrise and sunset
            const char *sunrise = doc["results"]["sunrise"];
            const char *sunset = doc["results"]["sunset"];

            Serial.print("Sunrise: ");
            Serial.println(sunrise);
            Serial.print("Sunset: ");
            Serial.println(sunset);

            // Convert sunrise to struct tm
            convertToTM(sunrise, sunrise_tm);
            // Convert sunset to struct tm
            convertToTM(sunset, sunset_tm);
        }
        else
        {
            Serial.println("HTTP request failed!");
            return false;
        }

        http.end(); // Close the connection
        return true;
    }
    return false;
}

// Function to check if it's day or night based on sunrise and sunset
void checkDayOrNight(struct tm *localTime, struct tm *sunrise_tm, struct tm *sunset_tm)
{
    // Convert times to seconds since midnight
    int local_seconds = timeToSecondsSinceMidnight(localTime);
    int sunrise_seconds = timeToSecondsSinceMidnight(sunrise_tm);
    int sunset_seconds = timeToSecondsSinceMidnight(sunset_tm);

    // Check if it's daytime or nighttime
    if (local_seconds >= sunrise_seconds && local_seconds <= sunset_seconds)
    {
        Serial.println("It's daytime. Plant light on");
        mode = _PLANT;
        setLeds(200, 90, 130);
    }
    else
    {
        Serial.println("It's nighttime. Plant light off.");
        mode = _DEFAULT;
        setLeds(0, 0, 0);
    }
}

bool initTime()
{
    struct tm timeinfo;
    struct tm sunset;
    struct tm sunrise;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to get time.");
        return 1;
    }
    if (!getSunsetSunrise(&sunrise, &sunset))
    {
        Serial.println("Failed to get sunset/sunrise.");
        return 1;
    }
    checkDayOrNight(&timeinfo, &sunrise, &sunset);
    return 0;
}

void dnsNext()
{
    // we only handle dns requests while the wifi manager is running
    dnsServer.processNextRequest();
}

int initWifi()
{
    if (loadCredentials())
    {
        return clientSetup();
    }
    else
    {
        managerSetup();
        return 2;
    }
}

// Setup Access Point mode with wifi manager
void managerSetup()
{
    Serial.println("[\e[0;32m  OK  \e[0;37m] Requesting Access Point");
    WiFi.softAPConfig(IPAddress(8, 8, 8, 8), IPAddress(8, 8, 8, 8),
                      IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID, AP_PW);
    Serial.println(INDENT + "Connect to: " + AP_SSID + " with password: " + AP_PW);
    localIP = WiFi.softAPIP();
    dnsServer.start(53, "*", localIP);
    hostManager();
}

// Setup station mode with credentials in ram
bool clientSetup()
{
    WiFi.mode(WIFI_STA);
    if (!(creds[_IPAD] == "" || creds[_GATE] == "" || creds[_SUBN] == ""))
    {
        localIP.fromString(creds[_IPAD].c_str());
        localGateway.fromString(creds[_GATE].c_str());
        localSubnet.fromString(creds[_SUBN].c_str());

        if (WiFi.config(localIP, localGateway, localSubnet))
        {
            Serial.println("[\e[0;32m  OK  \e[0;37m] Setting WiFi Config");
        }
        else
        {
            Serial.println("[\e[0;31mFAILED\e[0;37m] Setting WiFi Config");
            return false;
        }
    }
    Serial.print(INDENT + "Connecting to WiFi");
    WiFi.begin(creds[_SSID].c_str(), creds[_PASS].c_str());
    setLeds(15, 15, 0); // yellow
    for (int tries = 0; !(WiFi.status() == WL_CONNECTED); tries++)
    {
        delay(100);
        Serial.print(".");
        if (tries == 50) // connection unsuccessful
        {
            setLeds(15, 0, 0); // red
            delay(200);
            Serial.println();
            Serial.println("[\e[0;31mFAILED\e[0;37m] Connecting to WiFi");
            setLeds(0, 0, 0);
            return false;
        }
    }
    // successful connection
    setLeds(0, 15, 0); // green
    delay(200);
    hostIndex(); // host the usual webpage
    Serial.println();
    Serial.println("[\e[0;32m  OK  \e[0;37m] Connecting to WiFi");
    setLeds(0, 0, 0);
    return true;
}

/*############################## WEBSITE STUFF ##############################*/
void hostIndex()
{
    // Route for root / web page
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send(LittleFS, "/index.html", "text/html", false); });
    webServer.serveStatic("/", LittleFS, "/");

    // Answer GET request to /restart
    webServer.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send(200, "text/plain", "OK");
                 reset = true; });

    // Answer POST request to /color
    webServer.on("/color", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        if (request->hasParam(PARAM_R, true, false) &&
            request->hasParam(PARAM_G, true, false) &&
            request->hasParam(PARAM_B, true, false)) {
            r = request->getParam(PARAM_R, true, false)->value().toInt();
            g = request->getParam(PARAM_G, true, false)->value().toInt();
            b = request->getParam(PARAM_B, true, false)->value().toInt();
            setLeds(r, g, b);
            mode = _DEFAULT;
        }
        request->send(200, "text/plain", "OK"); });

    // Answer POST request to /mode
    webServer.on("/mode", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        if (request->hasParam(PARAM_MODE, true, false)) {
            mode = request->getParam(PARAM_MODE, true, false)->value().toInt();
            switch (mode) {
            case _OFF:
                setLeds(0, 0, 0);
                break;
            case _DEFAULT:
                setLeds(r, g, b);
                break;
            case _PLANT:
                setLeds(200, 90, 130);
            default:
                break;
            }
        }
        request->send(200, "text/plain", "OK"); });

    webServer.begin(); // start ws
}

void hostManager()
{
    webServer.onNotFound([](AsyncWebServerRequest *request)
                         {
        if (!handleFileRequest(request, request->url()))
            request->send(404, "text/plain", "File not found"); });

    // Catch for various captive portal default redirects
    webServer.on("/generate_204", [](AsyncWebServerRequest *request)
                 { request->redirect(MANAGER); }); // android captive portal
    webServer.on("/redirect", [](AsyncWebServerRequest *request)
                 { request->redirect(MANAGER); }); // microsoft redirect
    webServer.on("/hotspot-detect.html", [](AsyncWebServerRequest *request)
                 { request->redirect(MANAGER); }); // apple call home
    webServer.on("/mobile/status.php", [](AsyncWebServerRequest *request)
                 { request->redirect(MANAGER); }); // various call home

    webServer.on("/submit", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        for (uint8_t i = 0; i < request->params(); i++) {
            AsyncWebParameter* p = request->getParam(i);
            if (p->isPost()) {
                String paramName = p->name();
                String paramValue = p->value().c_str();
                uint8_t writeOffset;

                if (paramName == PARAM_INPUT_0) {
                    writeOffset = _SSID;
                } else if (paramName == PARAM_INPUT_1) {
                    writeOffset = _PASS;
                } else if (paramName == PARAM_INPUT_2) {
                    writeOffset = _IPAD;
                } else if (paramName == PARAM_INPUT_3) {
                    writeOffset = _GATE;
                } else if (paramName == PARAM_INPUT_4) {
                    writeOffset = _SUBN;
                } else {
                    continue; // Skip unrecognized parameters
                }
                creds[writeOffset] = paramValue;
                writeFile(paths[writeOffset], creds[writeOffset].c_str());
            }
        }

        String response = "Done. ESP will restart.";
        Serial.println(INDENT + response);
        request->send(200, "text/plain", response);
        setLeds(0, 255, 0);
        delay(150);
        setLeds(0, 0, 0);
        ESP.restart(); });
    webServer.begin();
}

bool handleFileRequest(AsyncWebServerRequest *request, String path)
{
    String contentType;
    if (path.endsWith("/"))
        path = "manager.html";
    if (path.endsWith(".html"))
        contentType = "text/html";
    if (path.endsWith(".css"))
        contentType = "text/css";
    if (LittleFS.exists(path))
    {
        AsyncWebServerResponse *response =
            request->beginResponse(LittleFS, path, contentType);
        request->send(response);
        return true;
    }
    return false;
}