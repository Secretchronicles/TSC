# -*- coding: utf-8; indent-tabs-mode: t -*-
#############################################################################
# Makefile  -  Make instruction file
#
# Copyright © 2012-2020 The TSC Contributors
#############################################################################
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# This Makefile contains helper tasks for the development of TSC.
# It is neither meant nor able to compile TSC. Refer to INSTALL.md
# for compilation instructions.

TSC_SIGNKEY     ?= "0000000"
TSC_VERSION     != grep 'set(TSC_VERSION' CMakeLists.txt | cut -d ' ' -f 2 | tr -d ')"' | xargs | tr ' ' . | sed -E 's/.([a-z]+)$$/-\1/'
TSC_TARFILE     := $(shell pwd)/../../TSC-$(TSC_VERSION).tar.gz

# Ensure users don't just blindly call make for compilation.
default:
	@echo "This is not a build Makefile and does not have a default task."
	@echo "Refer to INSTALL.md for TSC installation instructions."

# Removes build artifacts and automatically generated files.
clobber:
	rm -rf build
	find -iname '*~' -delete

# Creates the source tarball for distribution of TSC.
# The tarball is created in the parent's parent directory.
$(TSC_TARFILE):
	@echo "Packaging TSC version $(TSC_VERSION)"
	git submodule init
	git submodule update
	rm -f ../../TSC-$(TSC_VERSION)
	ln -s $(shell basename $(dir $(shell pwd))) ../../TSC-$(TSC_VERSION)
	rm -f $@
	tar -f $@ -C ../.. --exclude=.git --exclude=.gitignore --exclude=.gitmodules --exclude=tsc/build --exclude=org.secretchronicles.TSC.json --exclude=flatpak --exclude=.travis.yml --exclude='*.sh' --exclude drafts --exclude-backups --exclude-vcs-ignores -cvzh TSC-$(TSC_VERSION)
	rm ../../TSC-$(TSC_VERSION)
	@ls -hl $@

# Shorthand for calling the above target.
tarball: $(TSC_TARFILE)

# Builds all files required for a release of the TSC source code.
release: $(TSC_TARFILE)
	@echo ">>>>> Checking syntax of translation PO files"
	$(MAKE) -C data/translations checkpo
	@echo ">>>>> Compiling from the release tarball"
	rm -rf /tmp/tsc-release
	mkdir /tmp/tsc-release
	tar -C /tmp/tsc-release -xf $<
	mkdir /tmp/tsc-release/TSC-$(TSC_VERSION)/tsc/build
	cd /tmp/tsc-release/TSC-$(TSC_VERSION)/tsc/build && \
		cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/tmp/tsc-release/build -DCEGUI_USE_EXPAT=ON .. && \
		make -j$(shell nproc) && \
		make install
	@echo ">>>>> Running the game now. Test a bit and close when you are satisfied."
	/tmp/tsc-release/build/bin/tsc
	rm -rf /tmp/tsc-release
	@echo ">>>>> Signing release tarball with $(TSC_SIGNKEY) (set TSC_SIGNKEY in environment to change this)"
	gpg --local-user $(TSC_SIGNKEY) --detach-sign $<
	@echo ">>>>> Generating checksums"
	md5sum $< > ../../TSC-$(TSC_VERSION).tar.gz.md5sum
	sha256sum $< > ../../TSC-$(TSC_VERSION).tar.gz.sha256sum
	@echo ">>>>> Done. Release files are:"
	@ls -hl ../../TSC-$(VERSION)*

# Updates all copyright notices in the source tree to the current year.
update_copyright:
	find -type f -exec sed -i -E 's/Copyright (©|\(C\)|\(c\)) .* The TSC Contributors/Copyright © 2012-2020 The TSC Contributors/' '{}' ';'

# Runs the "optipng" programme on all .PNG files in the data/ directory.
# "optipng" must be installed for this. This should be done after new
# .PNG files were added. It reduces the size of the PNGs considerably.
optipng:
	find data -iname '*.png' -print | xargs -t -L 1 -P $(shell nproc) optipng -q -o4 --

.PHONY: default update_copyright tarball release optipng
