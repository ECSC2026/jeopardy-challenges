#!/bin/sh
set -e

gunicorn -b 127.0.0.1:5000 -w 1 --access-logfile - --error-logfile - app:app &

exec nginx -g 'daemon off;'
