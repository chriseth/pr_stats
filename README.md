## PR Status Indicator

This repository contains code to control an LED strip to
be used as a pull request status indicator.

There is one LED per pull request.

Its position on the strip indicates its age.

If it flashes red, it means that the pull request author should do something:
 - the tests are failing
 - there is a review that requests changes
 - the pull request has merge conflicts

If it flashes yellow, it means that someone should add a review.

## How to use

The code is set up to control a strip with 120 LEDs connected to port 13
of an ESP32 that logs into a local WiFi and retrieves the pull request
status from github.. The number of LEDs and the port can be changed in
``leds.h``.

You also need to configure some access codes in ``secrets.h``:

    #define WIFI_SSID "MY_SSID"
    #define WIFI_PASSWORD "PASSWORD"
    #define Github_token "token github_token"
    #define REPOSITORY_OWNER "ethereum"
    #define REPOSITORY_NAME "solidity"

## How to improved

 - it would be nice to have an animation/hilight when PRs change state
 - if PRs are closed, there could be an even nicer animation
 - the ESP32 should listen for a webhook call from github to update

