/***************************************************************************
 * spinner.h
 *
 * Copyright © 2010 - 2011 Florian Richter
 * Copyright © 2013 - 2014 The TSC Contributors
 ***************************************************************************/
/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TSC_SPINNER_HPP
#define TSC_SPINNER_HPP

#include "../core/global_basic.hpp"

namespace CEGUI {

    /* *** *** *** *** *** *** *** *** TSC_Spinner *** *** *** *** *** *** *** *** *** */

    class TSC_Spinner : public Spinner {
    public:
        TSC_Spinner(const String& type, const String& name);
        virtual ~TSC_Spinner(void);

        /*!
        \brief
            Returns the textual representation of the current spinner value.

        \return
            String object that is equivalent to the the numerical value of the spinner.
        */
        virtual String getTextFromValue(void) const;

        // Events
        //bool Mouse_Wheel( const CEGUI::EventArgs &event );

        static const String WidgetTypeName;
    };

    CEGUI_DECLARE_WINDOW_FACTORY(TSC_Spinner)

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace CEGUI

#endif
