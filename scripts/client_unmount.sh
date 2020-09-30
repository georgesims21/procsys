#!/bin/bash
mount="/home/vagrant/procsys"

if grep -qs "$mount" /proc/mounts; then
  fusermount -u ~/procsys 2>&1
else
    exit 0
fi
