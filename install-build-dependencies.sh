#!/bin/bash

# Requires Debian 9 or Ubuntu 16.10 or newer

# Install deb packages
sudo apt-get -y install ruby-full rake gperf pkg-config bison libglew-dev \
  freeglut3-dev gettext libpng-dev libpcre3-dev libxml++2.6-dev \
  libfreetype6-dev libdevil-dev libboost-all-dev libsfml-dev \
  libcegui-mk2-dev cmake build-essential git git-core libglm-dev

# Install Ruby gems
sudo gem install bundler nanoc adsf kramdown coderay
