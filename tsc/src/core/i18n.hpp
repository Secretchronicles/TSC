/***************************************************************************
 * i18n.h
 *
 * Copyright © 2008 - 2011 Florian Richter
 * Copyright © 2012-2020 The TSC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TSC_I18N_HPP
#define TSC_I18N_HPP

#include "../core/global_game.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

// translates the string with gettext
#define _(String) gettext(String)
// Like _(), but also tells CEGUI this is UTF-8.
#define UTF8_(String) reinterpret_cast<const CEGUI::utf8*>(gettext(String))
// not translated and only for gettext detection
#define N_(String) String
// translates with singular and plural
#define PL_(Singular, Plural, Num) ngettext((Singular), (Plural), (Num))
// translates with context where ambigous (see section 11.2.5 of
// the Gettext manual).
#define C_(Context, String) pgettext(Context, String)

// init internationalization
    void I18N_Init(void);
// set language
    void I18N_Set_Language(const std::string& default_language);

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif
