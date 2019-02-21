#pragma once

#include "pr.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include <vector>
#include <map>

#include <time.h>

struct ResponseParser
{
    static std::vector<PR> parseResponse(Stream& _stream);
};

namespace
{

int parseTime(String const& _time)
{
    tm timedata;
    char buf[sizeof(timedata)];

    memset(&timedata, 0, sizeof(timedata));
    if (strptime(_time.c_str(), "%Y-%m-%dT%H:%M:%SZ", &timedata) == nullptr)
        return 0;
    // TODO mktime assumes local time, but we just ignore it,
    // should not matter too much for sorting
    return mktime(&timedata);
}

Status mergeStatus(String const& _str)
{
    if (_str == "MERGEABLE")
        return Status::OK;
    else if (_str == "CONFLICTING")
        return Status::FAILURE;
    else if (_str == "PENDING")
        return Status::PENDING;
    else
        return Status::ERROR;
}

Status approval(JsonArray const& _reviews)
{
    std::map<String, Status> approvalByUser;
    for (auto const& review: _reviews)
    {
        JsonObject const& rc = review["node"];
        String user = rc["author"]["login"];
        String state = rc["state"];
        if (state == "COMMENTED")
            continue;
        else if (state == "DISMISSED")
            approvalByUser[user] = Status::UNKNOWN;
        else if (state == "APPROVED")
            approvalByUser[user] = Status::OK;
        else if (state == "CHANGES_REQUESTED")
            approvalByUser[user] = Status::FAILURE;
        else
            approvalByUser[user] = Status::ERROR;
    }
    Status overallApproval = Status::UNKNOWN;
    for (auto const& approval: approvalByUser)
        if (approval.second > overallApproval)
            overallApproval = approval.second;
    if (overallApproval == Status::UNKNOWN)
        overallApproval = Status::PENDING;

    return overallApproval;
}

Status tests(JsonArray const& _tests)
{
    Status overallState = Status::UNKNOWN;
    for (auto const& context: _tests)
    {
        String ctName = context["context"];
        if (!ctName.startsWith(F("ci/circleci")))
            continue;
        String state = context["state"];
        Status testState = Status::UNKNOWN;
        if (state == "SUCCESS")
            testState = Status::OK;
        else if (state == "PENDING")
            testState = Status::PENDING;
        else if (state == "FAILURE")
            testState = Status::FAILURE;
        else
            testState = Status::ERROR;
        if (testState > overallState)
            overallState = testState;
    }
    if (overallState == Status::UNKNOWN)
        overallState = Status::PENDING;
    return overallState;
}

}

std::vector<PR> ResponseParser::parseResponse(Stream& _stream)
{
    std::vector<PR> prs;

    Serial.println("Got stream.");
    if (!_stream.findUntil("[", ""))
    {
        Serial.println("Array not found.");
        return prs;
    }

    while (true)
    {
        StaticJsonBuffer<4000> jsonBuffer;
        JsonObject& root = jsonBuffer.parse(_stream);
        if (!root.success())
        {
            Serial.println("Error parsing PR.");
            break;
        }
        Serial.println("Parsed.");

        JsonObject& pr = root[F("node")];
        prs.emplace_back();
        PR& parsedPR = prs.back();
        parsedPR.id = pr["number"].as<int>();
        parsedPR.createdAt = parseTime(pr["createdAt"].as<String>());
        parsedPR.mergeable = mergeStatus(pr["mergeable"].as<String>());
        parsedPR.approval = approval(pr["reviews"]["edges"]);
        parsedPR.tests = tests(pr["commits"]["edges"][0]["node"]["commit"]["status"]["contexts"]);
        parsedPR.updateCombined();
        parsedPR.debugOut();

        if (_stream.read() != ',')
            break;
    }
    Serial.printf("Found %d PRs\n", prs.size());
    return prs;
}
