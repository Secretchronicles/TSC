/***************************************************************************
 * spinner.cpp  -  custom cegui spinner
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

#include "../gui/spinner.hpp"
#include "../core/game_core.hpp"

namespace CEGUI {

/* *** *** *** *** *** *** *** *** TSC_Spinner *** *** *** *** *** *** *** *** *** */

const String TSC_Spinner::WidgetTypeName("CEGUI/TSC_Spinner");

TSC_Spinner::TSC_Spinner(const String& type, const String& name)
    : Spinner(type, name)
{
    // todo : when cegui has a virtual Spinner::initialiseComponents
    //getEditbox()->subscribeEvent( Window::EventMouseWheel, Event::Subscriber( &TSC_Spinner::Mouse_Wheel, this ) );
}

TSC_Spinner::~TSC_Spinner(void)
{

}

String TSC_Spinner::getTextFromValue(void) const
{
    if (d_inputMode == FloatingPoint) {
        return TSC::float_to_string(d_currentValue, 6, 0);
    }

    return Spinner::getTextFromValue();
}

/*bool TSC_Spinner::Mouse_Wheel( const EventArgs &event )
{
    const MouseEventArgs &mouse_event = static_cast<const MouseEventArgs&>( event );

    setCurrentValue( d_currentValue + ( d_stepSize * mouse_event.wheelChange ) );

    return 1;
}*/

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace CEGUI
