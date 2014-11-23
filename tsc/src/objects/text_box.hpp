/***************************************************************************
 * text_box.h
 *
 * Copyright © 2007 - 2011 Florian Richter
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

#ifndef TSC_TEXT_BOX_HPP
#define TSC_TEXT_BOX_HPP

#include "../core/global_basic.hpp"
#include "../objects/box.hpp"

namespace TSC {

    /* *** *** *** *** *** *** *** *** cText_Box *** *** *** *** *** *** *** *** *** */

    class cText_Box : public cBaseBox {
    public:
        // constructor
        cText_Box(cSprite_Manager* sprite_manager);
        // create from stream
        cText_Box(XmlAttributes& attributes, cSprite_Manager* sprite_manager);
        // destructor
        virtual ~cText_Box(void);

        // init defaults
        void Init(void);
        // copy
        virtual cText_Box* Copy(void) const;

        // load from stream
        virtual void Load_From_XML(XmlAttributes& attributes);

        // Activate
        virtual void Activate(void);

        // update
        virtual void Update(void);

        // Set Text
        void Set_Text(const std::string& str_text);

        // editor activation
        virtual void Editor_Activate(void);
        // editor text text changed event
        bool Editor_Text_Text_Changed(const CEGUI::EventArgs& event);

        // the text
        std::string m_text;

        // Save to node
        virtual xmlpp::Element* Save_To_XML_Node(xmlpp::Element* p_element);

    protected:
        // typename inherited
    };

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif
