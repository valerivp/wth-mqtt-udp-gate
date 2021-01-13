

#include "webserver.h"

#include "Arduino.h"
#if defined(ESP8266)
#include <FS.h>
#endif // ESP

#include "common_store.h"

#include "esp_def.h"
#include "timers.h"


#if defined(ESP8266)
extern "C" {
#include "user_interface.h"
}

#endif // ESP


HTTPserverClass HTTPserver;

AsyncWebServerEx HTTPserverClass::server(80);
AsyncStaticWebHandler* HTTPserverClass::serveStaticHandlerNA = nullptr;

String HTTPserverClass::help_info;

enum ContentTypes : uint8_t {
	application_octet_stream = 0, text_html, text_css, application_javascript, image_png,
	image_gif, image_jpeg, image_x_icon, text_xml, application_x_pdf,
	application_x_zip, application_x_gzip, text_plain, text_json
};

const char * ContentTypesStrings[] = { "application/octet-stream", "text/html", "text/css", "application/javascript", "image/png",
									   "image/gif", "image/jpeg", "image/x-icon", "text/xml", "application/x-pdf",
										"application/x-zip", "application/x-gzip", "text/plain", "text/json" };




wl_status_t WiFi_waitForConnectResult(uint16_t timeout) {
	//1 and 3 have STA enabled
	if (!(WiFi.getMode() & WIFI_STA)) {
		return WL_DISCONNECTED;
	}
	ulong end = millis() + timeout;
	while (WiFi.status() == WL_DISCONNECTED && millis() < end) {
		delay(100);
		ESP_wdtFeed();
	}
	return WiFi.status();
}

wl_status_t WiFi_disconnect(uint16_t timeout) {
	//1 and 3 have STA enabled
	if (!(WiFi.getMode() & WIFI_STA)) {
		return WL_DISCONNECTED;
	}
	ulong end = millis() + timeout;
	while (WiFi.status() == WL_CONNECTED && millis() < end) {
		WiFi.disconnect();
		delay(100);
		ESP_wdtFeed();
	}
	return WL_DISCONNECTED;
}

void HTTPserverClass::handleSetWiFiConfig(AsyncWebServerRequest *request) {
	DEBUG_STACK();
#if defined(hasDemoMode)
	if (ConfigStore.getDemoMode())
		return return403_DemoMode(request);
#endif
	if (request->hasArg(F("ssid")) && request->hasArg(F("password"))) {
		String result;
		String ssid = request->arg(F("ssid"));
		String password = request->arg(F("password"));
		DEBUG_PRINT(F("ssid:") << ssid << F(",password:") << password);
		if (ssid.length()) {
			// задано имя точки доступа. отключимся
			DISABLE_TIMER_INTERRUPTS();
			DEBUG_PRINT(F("Try WiFi.enableAP"));
			WiFi.enableAP(true); // включим режим станции и точки доступа
			WiFi_disconnect(3000);

			if (password.length()) {
				// задано имя точки доступа и пароль. будем подключаться
				DEBUG_PRINT(F("Try WiFi.begin"));

				WiFi.begin(ssid.c_str(), password.c_str()); // пробуем подключиться

				DEBUG_PRINT(F("WiFi.begin"));
				wl_status_t connRes = WiFi_waitForConnectResult(3000);
				if (connRes != wl_status_t::WL_CONNECTED) {
					result = F("Not connected:");
					switch (connRes) {
					case wl_status_t::WL_NO_SSID_AVAIL:
						result += F("WL_NO_SSID_AVAIL"); break;
					case wl_status_t::WL_CONNECT_FAILED:
						result += F("WL_CONNECT_FAILED"); break;
					case wl_status_t::WL_DISCONNECTED:
						result += F("WL_DISCONNECTED"); break;
					default:
						result += connRes; break;
					};
				}
				else
					result = F("Connected");
			}
			else
				result = F("Disconected");
			// если подключимся - сработает обработчик события, и точка доступа отключится
			ENABLE_TIMER_INTERRUPTS();
		}
		else {
			result = F("SSID or password not set");
		}
		DEBUG_PRINT(result);
		request->send(200, ContentTypesStrings[ContentTypes::text_plain], result);
	}
	else
		request->send(400);
}

void HTTPserverClass::handleWiFiScan(AsyncWebServerRequest *request) {
	DEBUG_STACK();

	int8_t countNetworks = WiFi.scanComplete();
	DEBUG_PRINT(countNetworks);
	if (countNetworks == -2) {
		Timers.once([]() {/*ETS_FRC1_INTR_DISABLE();*/ WiFi.scanNetworks(true); /*ETS_FRC1_INTR_ENABLE();*/ }, 1);
		request->send(202, ContentTypesStrings[ContentTypes::text_plain], "202. Start scan. Please wait and refresh page..."); // no content
		return;
	}
	else if (countNetworks == -1) {
		request->send(202, ContentTypesStrings[ContentTypes::text_plain], "202. Still scanning..."); // no content
		return;
	}

	//DEBUG_PRINT(WiFi.scanNetworks());

	if (request->arg("format") == "json") {

		String output = "[";
		//		ETS_FRC1_INTR_DISABLE();
		for (int i = 0; i < countNetworks; i++) {
			if (output != "[") output += ',';
			output += "\n{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + WiFi.RSSI(i) + ",\"enc\":" + WiFi.encryptionType(i) + "}";
		}
		output += "\n]";
		//		ETS_FRC1_INTR_ENABLE();
		request->send(200, ContentTypesStrings[ContentTypes::text_json], output);

	}
	else {
		String output;
		//		ETS_FRC1_INTR_DISABLE();
		//DEBUG_PRINT(n);
		for (int i = 0; i < countNetworks; i++)
			output += WiFi.SSID(i) + "\t" + WiFi.RSSI(i) + "\t" + WiFi.encryptionType(i) + "\n";
		//		ETS_FRC1_INTR_ENABLE();
		request->send(200, ContentTypesStrings[ContentTypes::text_plain], output);
	}
	WiFi.scanDelete();

}

void HTTPserverClass::handleWiFiInfo(AsyncWebServerRequest *request) {
	DEBUG_STACK();


#if defined(ESP8266)
	softap_config config;

	wifi_softap_get_config(&config);

#elif defined(ESP32)
	wifi_config_t config_ap;
	esp_wifi_get_config(WIFI_IF_AP, &config_ap);
	auto config = config_ap.ap;

#endif

	char apname[sizeof(config.ssid) + 1];

	memset(apname, 0, sizeof(apname));
	strncpy(apname, (const char *)config.ssid, config.ssid_len ? sizeof(apname) : config.ssid_len);

	if (request->arg("format") == "json") {
		String output = F("{\n");
		output += String(F("\"mode\":")) + WiFi.getMode() + ",\n";
		output += String(F("\"ssid\":\"")) + WiFi.SSID() + "\",\n";
		output += String(F("\"ap\":\"")) + apname + "\"\n";
		output += String(F("\"MAC\":\"")) + WiFi.macAddress() + "\"\n";
		output += String(F("}"));
		request->send(200, ContentTypesStrings[ContentTypes::text_json], output);

	}else{
		String output;
		output += String(F("mode:")) + WiFi.getMode() + "\n";
		output += String(F("ssid:")) + WiFi.SSID() + "\n";
		output += String(F("ap:")) + apname + "\n";
		output += String(F("MAC:")) + WiFi.macAddress() + "\n";

		request->send(200, ContentTypesStrings[ContentTypes::text_plain], output);
	}
}

void HTTPserverClass::handleUpdateFlash(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
	//DEBUG_STACK();
#if defined(hasDemoMode)
	if (ConfigStore.getDemoMode())
		return return403_DemoMode(request);
#endif
#if defined(ESP8266)

	if (!index) {
		DEBUG_PRINT(F("Update Start:") << filename.c_str());
		Update.runAsync(true);
		if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
			Update.printError(Serial);
		}
	}
	DEBUG_PRINT(F("Update part:") << index);
	if (!Update.hasError()) {
		if (Update.write(data, len) != len) {
			Update.printError(Serial);
			request->send(400);
		}
	}
	if (final) {
		if (Update.end(true)) {
			//ESP.restart();
			DEBUG_PRINT(F("Update Success:") << (index + len));
			request->send(200, ContentTypesStrings[ContentTypes::text_plain], "Ok. Wait for restart");
			Timers.once([]() {ESP.restart(); }, 1000);
		}
		else {
			Update.printError(Serial);
			request->send(400);
		}
	}


#elif defined(ESP32)

	DEBUG_PRINT(F("ApplyRelayState not imlemented"));

#endif

	
}

void HTTPserverClass::handleInfo(AsyncWebServerRequest *request) {
	DEBUG_STACK();

	String output;
//	time_t uptime = Clock.uptime();

	if (request->arg("format") == "json") {
		String output = F("{\n");
		output += String(F("\"MCU\":\"ESP8266\"\n"));

		output += String(F("\"ChipID\":\"")) + ESP_getChipIdStr() + "\"\n";
		output += String(F("\"FreeHeap\":\"")) + String(ESP.getFreeHeap()) + "\"\n";
		output += String(F("\"Version\":\"")) + String(__DATE__) + "\"\n";
		//output += String(F("\"Uptime\":\"")) + String((int)(uptime / 86400ul)) + time2str(uptime, "D%H%M%S") + "\"\n";
		output += String(F("}"));
		request->send(200, ContentTypesStrings[ContentTypes::text_json], output);

	}
	else {
		String output;
		output += String(F("MCU:ESP8266\n"));
		output += String(F("ChipID:")) + ESP_getChipIdStr() + '\n';
		output += String(F("FreeHeap:")) + String(ESP.getFreeHeap()) + '\n';
		output += String(F("Version:")) + String(__TIMESTAMP__) + '\n';
		//output += String(F("Uptime:")) + String((int)(uptime / 86400ul)) + time2str(uptime, "D%H%M%S") + '\n';

		request->send(200, ContentTypesStrings[ContentTypes::text_plain], output);
	}

}

void HTTPserverClass::handleNetSetup_GET(AsyncWebServerRequest * request)
{
	DEBUG_STACK();
	char html[1024];
	sprintf_P(html, "<head><title>Config network</title></head><body><form method=\"post\" action=\"net-setup\">"
		"Mqtt/udp server: <input name = \"Server\" length=\"15\" placeholder=\"xxx.xxx.xxx.xxx\" value=\"%s\"><br>"
		"Mqtt/udp port: <input name = \"Port\" length=\"6\" placeholder=\"port\" value=\"%d\"><br>"
		"<button type = \"submit\">save</button></form></body></html>",
		ipaddr_ntoa(&ConfigStore.Data.MqttUdpServer),
		ConfigStore.Data.MqttUdpPort
	);
	request->send(200, ContentTypesStrings[ContentTypes::text_html], html);

}

void HTTPserverClass::handleNetSetup_POST(AsyncWebServerRequest * request)
{
	DEBUG_STACK();
	if (request->hasArg(F("Server")) && request->hasArg(F("Port"))) {

		ConfigStore.Data.MqttUdpServer.addr = ipaddr_addr(request->arg(F("Server")).c_str());
		ConfigStore.Data.MqttUdpPort = request->arg(F("Port")).toInt();
		ConfigStore.save();
		handleNetInfo(request);
	}
	else
		request->send(400);

}

void HTTPserverClass::handleNetInfo(AsyncWebServerRequest * request)
{
	DEBUG_STACK();
	char txt[1024];
	sprintf_P(txt, "Mqtt/udp server:%s\nMqtt/udp port:%d\n",
		ipaddr_ntoa(&ConfigStore.Data.MqttUdpServer), ConfigStore.Data.MqttUdpPort);
	request->send(200, ContentTypesStrings[ContentTypes::text_plain], txt);

}


void HTTPserverClass::init()
{
	

	help_info.concat(F("<br><a href=\"net-info\">/net-info</a>: set Network info. Allow methods: HTTP_GET\n"));
	server.on("/net-info", HTTP_GET, handleNetInfo);

	help_info.concat(F("<br><a href=\"net-setup\">/net-setup</a>: set Network setup. Allow methods: HTTP_GET, HTTP_POST\n"));
	const char* urlEM_net = "/net-setup";
	server.on(urlEM_net, HTTP_GET, handleNetSetup_GET);
	server.on(urlEM_net, HTTP_POST, handleNetSetup_POST);

	//info
	help_info.concat(F("<br><a href=\"info\">/info</a>: get system info. Allow methods: HTTP_GET\n"));
	server.on("/info", HTTP_GET, handleInfo);

	//simple WiFi config 
	help_info.concat(F("<br><a href=\"wifi\">/wifi</a>: edit wifi settings. Allow methods: HTTP_GET, HTTP_POST\n"));
	const char* urlWiFi = "/wifi";
	server.on(urlWiFi, HTTP_GET, [](AsyncWebServerRequest *request) { DEBUG_PRINT("/wifi"); request->send(200, ContentTypesStrings[ContentTypes::text_html], String(F("<head><title>Config WiFi</title></head><body><form method=\"post\" action=\"wifi\"><input name=\"ssid\" length=\"32\" placeholder=\"ssid\"><input name=\"password\" length=\"64\" type=\"password\" placeholder=\"password\"><button type=\"submit\">save</button></form></body></html>"))); });
	server.on(urlWiFi, HTTP_POST, handleSetWiFiConfig);

	//list wifi networks
	help_info.concat(F("<br><a href=\"wifi-scan\">/wifi-scan</a> ? [format=json]: get wifi list as text or json. Allow methods: HTTP_GET\n"));
	server.on("/wifi-scan", HTTP_GET, handleWiFiScan);

	help_info.concat(F("<br><a href=\"wifi-info\">/wifi-info</a> ? [format=json]: get wifi info as text or json. Allow methods: HTTP_GET\n"));
	server.on("/wifi-info", HTTP_GET, handleWiFiInfo);


	// update flash 
	help_info.concat(F("<br><a href=\"update\">/update</a>: update flash. Allow methods: HTTP_GET, HTTP_POST\n"));
	const char* urlUpdate = "/update";
	server.on(urlUpdate, HTTP_GET, [](AsyncWebServerRequest *request) { DEBUG_PRINT("/update"); request->send(200, ContentTypesStrings[ContentTypes::text_html], String(F("<head><title>Update</title></head><body><form action=\"/update\" method=\"POST\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"data\"><input type=\"submit\" value=\"update\"></form></body></html>"))); });
	server.on(urlUpdate, HTTP_POST, [](AsyncWebServerRequest *request) { DEBUG_PRINT(F("/update:POST")); request->send(200, ContentTypesStrings[ContentTypes::text_plain], String(F("File uploaded")));}, handleUpdateFlash);

	// update flash 
	help_info.concat(F("<br><a href=\"restart\">/restart</a>: restart system. Allow methods: HTTP_GET, HTTP_POST\n"));
	const char* urlRestart = "/restart";
	server.on(urlRestart, HTTP_GET, [](AsyncWebServerRequest *request) { DEBUG_PRINT("/restart"); request->send(200, ContentTypesStrings[ContentTypes::text_html], String(F("<head><title>Restart</title></head><body><form action=\"/restart\" method=\"POST\" \"><input type=\"submit\" value=\"restart\"></form></body></html>"))); });
	server.on(urlRestart, HTTP_POST, [](AsyncWebServerRequest *request) { DEBUG_PRINT(F("/restart:POST")); request->send(200, ContentTypesStrings[ContentTypes::text_plain], "Ok. Wait for restart"); Timers.once([]() {ESP.restart(); }, 1000);});

	//help_info.concat(F("<br><a href=\"help\">/help</a>: list allow URLs. Allow methods: HTTP_GET\n"));
	//server.on("/help", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, ContentTypesStrings[ContentTypes::text_plain], help_info.c_str()); });
	
	server.onNotFound([](AsyncWebServerRequest *request) {
		String res = "<html><head><title>";
		res += WiFi.hostname();
		res += "</title></head><body>";
		res.concat(help_info);
		res += "</body></html>";
		request->send(200, ContentTypesStrings[ContentTypes::text_html], res.c_str());
	});


	DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

	server.begin();

	DEBUG_PRINT(F("HTTP server started"));

}
