The Secret Chronicles of Dr. M.
===============================

“The Secret Chronicles of Dr. M.” is a 2D sidecrolling platform game,
with a rich set of graphics, music, and an advanced level editor that
allows you to create your own levels. The level editor allows for
in-game scripting so there are no borders apart from your imagination.

The project is a fork of [SMC](http://www.secretmaryo.org), which is
not developed actively anymore. Note this is not merely a continuation
of SMC, but we have our own goals and design principles we are slowly
integrating into both the codebase and the artwork.

How to install?
---------------

Releases are published precompiled for Windows at the website. If you
want to compile TSC yourself, please see the [INSTALL.md](INSTALL.md) file.

Links
-----

* The TSC website: https://secretchronicles.org
* The mailing list: https://lists.secretchronicles.org/postorius/lists/tsc-devel.lists.secretchronicles.org/
* The forum: https://lists.secretchronicles.org/hyperkitty/list/tsc-devel@lists.secretchronicles.org/
* The wiki: https://wiki.secretchronicles.org
* The bugtracker: https://github.com/Secretchronicles/TSC/issues

Contributing
------------

Any contributions to the code, the graphics, the music, etc. are
greatly appreciated. However, before starting your work on the game,
please consider the following:

* You have to be familiar with at least the basics of using the Git
  version control system. This can be achieved by reading the first
  two chapters of [this great online Git
  book](http://git-scm.com/book). Also reading chapter 3 is highly
  recommended as we use branches all the time.
* If you want to contribute code, please read the coding
  conventions document in tsc/docs/pages/conventions.md.
* If you want to contribute artistic work, please read [the styleguide](https://wiki.secretchronicles.org/StyleGuide)
* If you specifically target issues from the issue tracker, please
  use “fixes #43” for bug tickets you fix and “closes #43” for other
  tickets you resolve as part of the message in your commits. This
  causes GitHub to automatically close the corresponding ticket if
  we merge your changes.

Custom local configurations are provided for Emacs and ViM. In order for local
ViM configurations to work, you will need the localvimrc plugin, which can be
installed with the following command on most Bourne-compatible shells:

~~~sh
mkdir -p ~/.vim/plugin
cd ~/.vim/plugin
wget https://raw.githubusercontent.com/embear/vim-localvimrc/master/plugin/localvimrc.vim
~~~

License
-------

TSC is a two-dimensional jump’n’run platform game.

Copyright © 2003-2011 Florian Richter

Copyright © 2012-2019 The TSC Contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

* _See COPYING for the full license text._
* _See tsc/docs/authors.txt for the full list of authors._

Graphics, music, and any other artistic content in the game is _not_
licensed under the GPL. These assets undergo different terms as
outlined as follows: they are normally licensed under some kind of CC
license, as specified by an accompanying `.txt` or `.settings` file
to an asset that gives both the author(s) and the license of the asset
in question. Those assets whose accompanying `.txt` or `.settings`
file does not specify a license are licensed under the old SMC
Contribution license, which is included as the file
`tsc/docs/pages/old_smc_contribution_license.md`. Those assets were
inherited from SMC when TSC was forked off it, and we are looking for
a graphics artist to replace them with original art.
