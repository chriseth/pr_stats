#pragma once

#include "pr.h"
#include "networking.h"
#include "github_response_parser.h"

#include <vector>

struct PRs
{
    void setup();
    void update();
    std::vector<PR> prs;
    int previousUpdate = 0;
};

void PRs::setup()
{
    Networking::setup();
}

void PRs::update()
{
    int now = time(nullptr);
    if (now < previousUpdate + 20)
        return;
    previousUpdate = now;

    prs.clear();
    Networking::update([&](Stream& _stream) { prs = ResponseParser::parseResponse(_stream); });
    std::sort(prs.begin(), prs.end());
}