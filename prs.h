#pragma once

#include "pr.h"
#include "networking.h"
#include "github_response_parser.h"

#include <vector>

struct PRs
{
    void setup();
    void update();

    // Retrieve list of PRs from other thread.
    int updatedPRs(int _sequence, std::vector<PR>& _prs);

private:
    Networking m_networking;
    std::vector<PR> m_prs;
    SemaphoreHandle_t m_mutex = xSemaphoreCreateMutex();
    int m_updateSequence = 0;
    int m_previousUpdate = 0;
};

void PRs::setup()
{
    Serial.begin(115200);
    // Serial.setDebugOutput(true);
    while (!Serial) {};

    m_networking.setup();
}

void PRs::update()
{
    int now = time(nullptr);
    if (!m_networking.updateRequested() && now < m_previousUpdate + 120)
        return;

    Serial.println("Running updated.");
    m_previousUpdate = now;

    std::vector<PR> newPRs;
    Networking::update([&](Stream& _stream) { newPRs = ResponseParser::parseResponse(_stream); });
    std::sort(newPRs.begin(), newPRs.end());
    xSemaphoreTake(m_mutex, 2000);
    m_prs = std::move(newPRs);
    m_updateSequence++;
    xSemaphoreGive(m_mutex);
}

int PRs::updatedPRs(int _sequence, std::vector<PR>& _prs)
{
    int newSequence;
    xSemaphoreTake(m_mutex, 2000);
    newSequence = m_updateSequence;
    if (newSequence != _sequence)
        _prs = m_prs;
    xSemaphoreGive(m_mutex);
    return newSequence;
}