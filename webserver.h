#ifndef SERVER_H
#define SERVER_H

#include <arduino.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>



#include "common_def.h"



class AsyncWebServerEx : public AsyncWebServer {
	// in ESPAsyncWebServer-master\src\WebHandlerImpl.h
	// need add:
	// ...handleRequest(...)
	// if ((_username != "" && _password != "") && !request->authenticate(_username.c_str(), _password.c_str()))
	//	  return request->requestAuthentication();

public:
	AsyncWebServerEx(uint16_t port) : AsyncWebServer(port) {};

};

class HTTPserverClass {
public:
private:

	static String help_info;
	static AsyncWebServerEx server;

	static void handleInfo(AsyncWebServerRequest *request);

private:
	static void handleSetWiFiConfig(AsyncWebServerRequest *request);
	static void handleWiFiScan(AsyncWebServerRequest *request);
	static void handleWiFiInfo(AsyncWebServerRequest *request);
	static void handleSetAPConfig(AsyncWebServerRequest *request);
	static void handleSetUserConfig(AsyncWebServerRequest *request);
	static void handleUserInfo(AsyncWebServerRequest *request);

	static void handleUpdateFlash(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);

	static void handleNetSetup_GET(AsyncWebServerRequest *request);
	static void handleNetSetup_POST(AsyncWebServerRequest *request);
	static void handleNetInfo(AsyncWebServerRequest *request);



private:
	static AsyncStaticWebHandler* serveStaticHandlerNA;

public:
	static void init();

};

extern HTTPserverClass HTTPserver;

#endif //SERVER_H
