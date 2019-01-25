#!/bin/sh

if [ $# -ne 2 ]
then
  echo "Usage: $0 <hostname> <github api token>"
  exit 1
fi

ssh -R "$1".serveo.net:80:localhost:8888 serveo.net &
sleep 2
while true
do
  echo "Updating..."
  ./github_pr.py "$2" /dev/ttyACM*
  # Wait for webhook for no more than 1 minute
  ./webhook_wait.py 8888 60
done
