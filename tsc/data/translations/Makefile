# -*- coding: utf-8; indent-tabs-mode: t -*-
#############################################################################
# Makefile for translations
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

TSC_SOURCE_FILES = $(shell find ../../src -iregex '.*\.[ch]pp$$' -print0 | xargs -0)
PO_FILES = $(wildcard *.po)

checkpo:
	for pofile in $(PO_FILES); do echo -n "$$pofile: " &&  msgfmt -o /dev/null -c --statistics $$pofile; done

# Note that strictly the .po files depend on the .pot file, but the
# .pot file should only ever be updated on purpose and not as part of
# updating the .po files, as it is the reference file for all
# translators and thus must stay the same. Changes in it require all
# translators to review their translation. Also, it is likely that
# translators will not send in the changed .pot file. Therefore this
# task is not a file task on the .pot file; the .po files depend on
# the .pot file, and it would be autocalled that way when calling
# make on a .po file.
potfile:
	xgettext \
		--add-comments="TRANS:" \
		--language=C++ \
		--escape \
		--from-code=UTF-8 \
		--keyword=_ \
		--keyword=N_ \
		--keyword=UTF8_ \
		--keyword=PL_:1,2 \
		--keyword=C_:1c,2 \
		--copyright-holder='The TSC Contributors' \
		--package-name='Secretchronicles' \
		$(TSC_SOURCE_FILES) -o Secretchronicles.pot

%.po: Secretchronicles.pot
	msgmerge -U $@ $<

.PHONY: potfile checkpo
