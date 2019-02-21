#pragma once

#include <Arduino.h>

enum class PRState
{
    MERGEABLE,
    WAITING_FOR_TESTS,
    AUTHOR,
    REVIEWER,
    ERROR
};


enum class Status
{
    UNKNOWN,
    OK,
    PENDING,
    FAILURE,
    ERROR
};

struct PR
{
    int id;
    int createdAt;
    Status mergeable;
    Status approval;
    Status tests;
    PRState combinedStatus;

    void updateCombined();

    bool operator<(PR const& _other) const {
        return createdAt > _other.createdAt;
    }

    void debugOut() {
        Serial.printf("ID: %d\n", id);
        Serial.printf("Created at: %d\n", createdAt);
        Serial.printf("Mergeable: %d\n", mergeable);
        Serial.printf("Approval: %d\n", approval);
        Serial.printf("Tests: %d\n", tests);
        Serial.printf("PR Status: %d\n", combinedStatus);
    }
};


void PR::updateCombined()
{
    if (mergeable == Status::FAILURE)
        combinedStatus = PRState::AUTHOR;
    else if (mergeable != Status::ERROR)
    {
        if (tests == Status::FAILURE || approval == Status::FAILURE)
            combinedStatus = PRState::AUTHOR;
        else if (tests == Status::PENDING)
            combinedStatus = PRState::WAITING_FOR_TESTS;
        else if (tests == Status::OK)
        {
            if (approval == Status::OK)
                combinedStatus = PRState::MERGEABLE;
            else if (approval == Status::PENDING)
                combinedStatus = PRState::REVIEWER;
            else
                combinedStatus = PRState::ERROR;
        }
        else
            combinedStatus = PRState::ERROR;
    }
    else
        combinedStatus = PRState::ERROR;
}
