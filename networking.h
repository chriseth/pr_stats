#pragma once

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <HTTPClient.h>

#include <WiFiClientSecure.h>

#include <time.h>

#include <memory>

#include "secrets.h"

struct WebhookClient;

struct Networking
{
    void setup();

    bool updateRequested();

    template <class T>
    static void update(T const& _processStream);

private:
    static void setClock();
    void checkConnection();

    std::shared_ptr<WiFiServer> m_server;
    std::vector<std::shared_ptr<WebhookClient>> m_clients;
};

struct WebhookClient
{
public:
    explicit WebhookClient(WiFiClient _client):
        m_client(std::move(_client))
    {}

    void handle();
    bool updateRequested() { return m_updateRequested; }
    bool finished() { return !m_client; }

private:
    std::string m_data;
    int m_status = 0;
    bool m_updateRequested = false;
    WiFiClient m_client;
};


const char *rootCACertificate = R"(-----BEGIN CERTIFICATE-----
MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j
ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL
MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3
LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug
RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm
+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW
PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM
xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB
Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3
hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg
EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF
MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA
FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec
nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z
eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF
hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2
Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe
vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep
+OkuE6N36B9K
-----END CERTIFICATE-----)";

// Not sure if WiFiClientSecure checks the validity date of the certificate.
// Setting clock just to be sure...
void Networking::setClock()
{
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    Serial.println(F("Waiting for NTP time sync: "));
    time_t nowSecs = time(nullptr);
    while (nowSecs < 8 * 3600 * 2)
    {
        delay(500);
        Serial.print(F("."));
        yield();
        nowSecs = time(nullptr);
    }

    Serial.println();
    struct tm timeinfo;
    gmtime_r(&nowSecs, &timeinfo);
    Serial.println(F("Current time: "));
    Serial.println(asctime(&timeinfo));
}

void Networking::setup()
{
    Serial.println("Waiting for WiFi to connect...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.println(".");
        delay(1000);
    }
    Serial.println(" connected");
    Serial.println("Address: ");
    Serial.println(WiFi.localIP());

    setClock();

    m_clients.resize(5);

    m_server = std::make_shared<WiFiServer>();
    m_server->setNoDelay(false);
    m_server->begin();
}

void Networking::checkConnection()
{
    if (m_server->hasClient())
    {
        WiFiClient c = m_server->available();
        for (size_t i = 0; i < m_clients.size(); ++i)
            if (!m_clients[i])
            {
                Serial.println("Accepting client...");
                m_clients[i] = std::make_shared<WebhookClient>(std::move(c));
                return;
            }
        Serial.println("Max clients reached.");
        c.stop();
    }
}

bool Networking::updateRequested()
{
    checkConnection();

    bool ret = false;
    for (size_t i = 0; i < m_clients.size(); ++i)
    {
        if (!m_clients[i])
            continue;
        m_clients[i]->handle();
        if (m_clients[i]->updateRequested())
            ret = true;
        if (m_clients[i]->finished())
            m_clients[i] = nullptr;
    }
    return ret;
}

template <class T>
void Networking::update(T const& _processStream)
{
    std::shared_ptr<WiFiClientSecure> client = std::make_shared<WiFiClientSecure>();
    if (!client)
    {
        Serial.println("Unable to create client");
        return;
    }
    client->setCACert(rootCACertificate);

    HTTPClient https;

    Serial.println("Retrieving PRs.");
    if (!https.begin(*client, F("https://api.github.com/graphql")))
    {
        Serial.println("!!! Unable to connect.");
        return;
    }
    https.addHeader(F("Authorization"), F(Github_token));
    https.addHeader(F("Content-Type"), F("application/json"));
    int httpCode = https.POST(
        F(R"({"query":"{ repository(owner:\")" REPOSITORY_OWNER R"(\", name:\")" REPOSITORY_NAME R"(\") { pullRequests(last:40, states:OPEN, orderBy: {field: CREATED_AT, direction: ASC}) { edges { node { number title url mergeable createdAt commits(last:1) { edges { node { commit { status { state contexts { state context } }}}}} reviews(last:20, states:[APPROVED,CHANGES_REQUESTED,DISMISSED,PENDING]) { edges { node { author { login } state } } } } } } }}"})")
    );
    if (httpCode > 0)
    {
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {
            Serial.printf("Response size: %d\n", https.getSize());
            _processStream(https.getStream());
        }
    }
    else
    {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        String payload = https.getString();
        Serial.println(payload);
    }

    Serial.println("Update finished.");
    https.end();
}

void WebhookClient::handle()
{
    while (m_client.available() && m_status != 0x22)
    {
        int c = m_client.read();
        if (c == '\r')
            m_status += 0x10;
        else if (c == '\n')
            m_status += 0x01;
        else
            m_status = 0;
    }
    if (m_status != 0x22)
        return;
    Serial.println("Sending response...");
    m_client.println("HTTP/1.0 200 OK");
    m_client.println("Content-Type: text/plain");
    m_client.println("Connection: Closed");
    m_client.println("");
    m_client.println("Updating...");
    m_client.stop();
    m_updateRequested = true;
}
