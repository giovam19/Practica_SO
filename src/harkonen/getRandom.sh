#!/bin/bash

pid=$(pgrep -u giovanni.vecchies 'Fremen' | sort -R | tail -1)

echo "$pid"
