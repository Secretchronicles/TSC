#!/bin/sh
#############################################################################
# optipng.sh  -  Runs optipng on all png files in the current directory
#
# Copyright © 2003-2011 Florian Richter
# Copyright © 2013-2014 The TSC Contributors
#############################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

total_size() {
	find . -name \*.png -exec du -k {} \; | {
		while read kb foo ; do
			total=$((${total} + ${kb}))
		done
		echo ${total}
	}
}

optipng_r() {
	local opts="${*:--q -o4}" old_size=`total_size`

	find . -name \*.png | while read f ; do
		echo "optipng ${opts} ${f}"
		optipng ${opts} -- "${f}" || return 1
	done || return 1

	echo
	echo "Estimated reduction: $((${old_size} - `total_size`))K"
}

optipng_r "${*}"
