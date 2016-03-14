#!/bin/bash

ssh-keygen -t rsa -b 4096 -C "nofterator@gmail.com"

cat ~/.ssh/id_rsa.pub