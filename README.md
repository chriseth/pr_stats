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

It is built to have a driver (``run.sh``, ``webhook_wait.py`` and ``github_pr.py``)
to be run on a host computer (a Raspberry PI is perfect) and an LED controller
to be run on an Arduino (``pr_stats.ino``) connected via USB.

The code is set up to control a strip with 120 LEDs connected to port 5, but that
can be easily changed via the macros at the top of ``pr_stats.ino``.

The main command is the file ``run.sh`` which establishes an update cycle
by waiting for a github webook and then calling the ``github_pr.py`` script
to actually retrieve the data and send it to the Arduino.

You have to supply a host name for the github webhook and a github
api token. The script uses the service at http://serveo.net
to forward ``https://<hostname>.serveo.net`` to the local port 8888, where
``webhook_wait.py`` is waiting for a webhook call from github. As soon as
that happened (or after one minute), it calls ``github_pr.py`` to perform
the update and then waits again.