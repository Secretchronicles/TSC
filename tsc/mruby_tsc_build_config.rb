# -*- coding: utf-8 -*-
#############################################################################
# mruby_tsc_build_config.rb  -  Build configuration file for mruby
#
# Copyright © 2012-2017 The TSC Contributors
#############################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Absolute path to the directory this file resides in,
# independent of any PWD or invocation stuff.
THIS_DIR = File.expand_path(File.dirname(__FILE__))
# Absolute path mruby resides in
MRUBY_DIR = File.join(THIS_DIR, "..", "mruby", "mruby")

config = lambda do |conf, root|
  # The gems to build from mruby's standard collection. The binaries
  # (mruby, mirb) are not build, as well as some rather unsual parts
  # of mruby that are not needed.
  conf.gem :core => "mruby-print"
  conf.gem :core => "mruby-sprintf"
  conf.gem :core => "mruby-math"
  conf.gem :core => "mruby-time"
  conf.gem :core => "mruby-struct"
  conf.gem :core => "mruby-sleep"
  conf.gem :core => "mruby-enum-ext"
  conf.gem :core => "mruby-string-ext"
  conf.gem :core => "mruby-numeric-ext"
  conf.gem :core => "mruby-array-ext"
  conf.gem :core => "mruby-hash-ext"
  conf.gem :core => "mruby-range-ext"
  conf.gem :core => "mruby-proc-ext"
  conf.gem :core => "mruby-symbol-ext"
  conf.gem :core => "mruby-random" # replace with C++11 random-based method?
  conf.gem :core => "mruby-object-ext"
  conf.gem :core => "mruby-kernel-ext"
end

MRuby::Build.new do |conf|
  toolchain :gcc

  ## TSC_BUILD_TYPE comes from ProvideMRuby.cmake
  if ENV["TSC_BUILD_TYPE"] =~ /Debug/i
    enable_debug
  end

  # Define MRB_UTF8_STRING to make mruby's String class able
  # to handle non-ascii characters properly.
  conf.cc do |cc|
    cc.flags += ["-DMRB_UTF8_STRING"]
  end

  config.call(conf, root)
end
