/***************************************************************************
 * bonusbox.cpp  -  class for bonusbox
 *
 * Copyright © 2003 - 2011 Florian Richter
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

#include "../objects/bonusbox.hpp"
#include "../objects/star.hpp"
#include "../gui/hud.hpp"
#include "../level/level_player.hpp"
#include "../audio/audio.hpp"
#include "../core/framerate.hpp"
#include "../level/level.hpp"
#include "../level/level_editor.hpp"
#include "../core/game_core.hpp"
#include "../objects/goldpiece.hpp"
#include "../core/sprite_manager.hpp"
#include "../core/i18n.hpp"

namespace TSC {

/* *** *** *** *** *** *** *** *** cBonusBox *** *** *** *** *** *** *** *** *** */

cBonusBox::cBonusBox(cSprite_Manager* sprite_manager)
    : cBaseBox(sprite_manager)
{
    cBonusBox::Init();
}

cBonusBox::cBonusBox(XmlAttributes& attributes, cSprite_Manager* sprite_manager)
    : cBaseBox(sprite_manager)
{
    cBonusBox::Init();
    cBonusBox::Load_From_XML(attributes);
}

cBonusBox::~cBonusBox(void)
{
    for (MovingSpriteList::iterator itr = m_active_items.begin(); itr != m_active_items.end(); ++itr) {
        delete *itr;
    }

    m_active_items.clear();
}

void cBonusBox::Init(void)
{
    m_type = TYPE_BONUS_BOX;
    m_name = "Bonus Box";
    m_force_best_item = 0;
    m_camera_range = 5000;
    m_can_be_on_ground = 0;

    Set_Animation_Type("Bonus");
    m_gold_color = COL_DEFAULT;
    Set_Goldcolor(COL_YELLOW);
}

cBonusBox* cBonusBox::Copy(void) const
{
    cBonusBox* bonusbox = new cBonusBox(m_sprite_manager);
    bonusbox->Set_Pos(m_start_pos_x, m_start_pos_y);
    bonusbox->Set_Animation_Type(m_anim_type);
    bonusbox->Set_Bonus_Type(box_type);
    bonusbox->Set_Invisible(m_box_invisible);
    bonusbox->Set_Force_Best_Item(m_force_best_item);
    bonusbox->Set_Goldcolor(m_gold_color);
    bonusbox->Set_Useable_Count(m_start_useable_count, 1);
    return bonusbox;
}

void cBonusBox::Load_From_XML(XmlAttributes& attributes)
{
    cBaseBox::Load_From_XML(attributes);

    // item
    Set_Bonus_Type(static_cast<SpriteType>(string_to_int(attributes["item"])));
    // force best possible item
    Set_Force_Best_Item(string_to_int(attributes["force_best_item"]));
    // gold color
    if (box_type == TYPE_GOLDPIECE)
        Set_Goldcolor(Get_Color_Id(attributes.fetch("gold_color", Get_Color_Name(m_gold_color))));
}

xmlpp::Element* cBonusBox::Save_To_XML_Node(xmlpp::Element* p_element)
{
    xmlpp::Element* p_node = cBaseBox::Save_To_XML_Node(p_element);

    // force best possible item
    Add_Property(p_node, "force_best_item", m_force_best_item);
    // gold color
    if (box_type == TYPE_GOLDPIECE)
        Add_Property(p_node, "gold_color", Get_Color_Name(m_gold_color));

    return p_node;
}

void cBonusBox::Set_Useable_Count(int count, bool new_startcount /* = 0 */)
{
    cBaseBox::Set_Useable_Count(count, new_startcount);

    // disable
    if (!m_useable_count) {
        Reset_Animation();
        Set_Animation(0);
        Set_Image_Num(0);
    }
    // enable
    else {
        Set_Animation(1);
    }

    Update_Valid_Update();
}

void cBonusBox::Set_Bonus_Type(SpriteType bonus_type)
{
    // already set
    if (box_type == bonus_type) {
        return;
    }

    box_type = bonus_type;

    // set item image
    if (box_type == TYPE_UNDEFINED) {
        m_item_image = NULL;
    }
    else if (box_type == TYPE_POWERUP) {
        // force always best item
        m_force_best_item = 1;
        m_item_image = pVideo->Get_Surface("game/editor/unknown.png");
    }
    else if (box_type == TYPE_MUSHROOM_DEFAULT) {
        m_item_image = pVideo->Get_Surface("game/items/mushroom_red.png");
    }
    else if (box_type == TYPE_FIREPLANT) {
        m_item_image = pVideo->Get_Surface("game/items/fireberry_1.png");
    }
    else if (box_type == TYPE_MUSHROOM_BLUE) {
        m_item_image = pVideo->Get_Surface("game/items/mushroom_blue.png");
    }
    else if (box_type == TYPE_MUSHROOM_GHOST) {
        m_item_image = pVideo->Get_Surface("game/items/mushroom_ghost.png");
    }
    else if (box_type == TYPE_MUSHROOM_LIVE_1) {
        m_item_image = pVideo->Get_Surface("game/items/mushroom_green.png");
    }
    else if (box_type == TYPE_STAR) {
        m_item_image = pVideo->Get_Surface("game/items/star.png");
    }
    else if (box_type == TYPE_GOLDPIECE) {
        if (m_gold_color == COL_RED) {
            m_item_image = pVideo->Get_Surface("game/items/goldpiece/red/1.png");
        }
        else {
            m_item_image = pVideo->Get_Surface("game/items/goldpiece/yellow/1.png");
        }
    }
    else if (box_type == TYPE_MUSHROOM_POISON) {
        m_item_image = pVideo->Get_Surface("game/items/mushroom_poison.png");
    }
    else {
        m_item_image = NULL;
        printf("Error : Unknown BonusBox Item type : %d\n", box_type);
    }
}

void cBonusBox::Set_Force_Best_Item(bool enable)
{
    // can't be set if random
    if (box_type == TYPE_POWERUP) {
        return;
    }

    m_force_best_item = enable;
}

void cBonusBox::Set_Goldcolor(DefaultColor new_color)
{
    // already set
    if (m_gold_color == new_color) {
        return;
    }

    if (new_color == COL_DEFAULT) {
        m_gold_color = COL_YELLOW;
    }

    m_gold_color = new_color;

    if (box_type != TYPE_GOLDPIECE) {
        return;
    }

    if (m_gold_color == COL_YELLOW) {
        m_item_image = pVideo->Get_Surface("game/items/goldpiece/yellow/1.png");
        m_name = _("Box Goldpiece Yellow");
    }
    else if (m_gold_color == COL_RED) {
        m_item_image = pVideo->Get_Surface("game/items/goldpiece/red/1.png");
        m_name = _("Box Goldpiece Red");
    }
    else {
        printf("Warning : Unknown Bonusbox Gold Color %d\n", m_gold_color);
    }
}

void cBonusBox::Activate(void)
{
    cBaseBox::Activate();

    bool random = 0;

    // random
    if (box_type == TYPE_POWERUP) {
        int r = rand() % 5;

        if (r == 0) {
            box_type = TYPE_MUSHROOM_DEFAULT;
        }
        else if (r == 1) {
            box_type = TYPE_FIREPLANT;
        }
        else if (r == 2) {
            box_type = TYPE_MUSHROOM_BLUE;
        }
        else if (r == 3) {
            box_type = TYPE_MUSHROOM_GHOST;
        }
        else if (r == 4) {
            box_type = TYPE_STAR;
        }

        random = 1;
    }

    Maryo_type current_maryo_type;

    // use original type
    if (pLevel_Player->m_maryo_type == MARYO_GHOST) {
        current_maryo_type = pLevel_Player->m_maryo_type_temp_power;
    }
    // already using original type
    else {
        current_maryo_type = pLevel_Player->m_maryo_type;
    }

    // the item from this box
    cMovingSprite* box_item = NULL;

    // no item
    if (box_type == TYPE_UNDEFINED) {
        pAudio->Play_Sound("item/empty_box.wav");
    }
    // check if lower item should be used if no force best item
    else if (!m_force_best_item && (box_type == TYPE_FIREPLANT || box_type == TYPE_MUSHROOM_BLUE) &&
             (current_maryo_type == MARYO_SMALL || ((current_maryo_type == MARYO_FIRE || current_maryo_type == MARYO_ICE) && !pHud_Itembox->m_item_id))) {
        pAudio->Play_Sound("sprout_1.ogg");

        cMushroom* mushroom = new cMushroom(m_sprite_manager);
        mushroom->Set_Pos(m_start_pos_x - ((m_item_image->m_w - m_rect.m_w) / 2), m_start_pos_y - ((m_item_image->m_h - m_rect.m_h) / 2), 1);
        box_item = static_cast<cMovingSprite*>(mushroom);
    }
    else if (box_type == TYPE_FIREPLANT) {
        pAudio->Play_Sound("sprout_1.ogg");
        box_item = static_cast<cMovingSprite*>(new cFirePlant(m_sprite_manager));
        box_item->Set_Pos(m_start_pos_x - ((m_item_image->m_w - m_rect.m_w) / 2), m_start_pos_y, 1);
    }
    else if (box_type == TYPE_MUSHROOM_DEFAULT || box_type == TYPE_MUSHROOM_LIVE_1 || box_type == TYPE_MUSHROOM_POISON || box_type == TYPE_MUSHROOM_BLUE || box_type == TYPE_MUSHROOM_GHOST) {
        pAudio->Play_Sound("sprout_1.ogg");

        cMushroom* mushroom = new cMushroom(m_sprite_manager);
        mushroom->Set_Pos(m_start_pos_x - ((m_item_image->m_w - m_rect.m_w) / 2), m_start_pos_y - ((m_item_image->m_h - m_rect.m_h) / 2), 1);
        mushroom->Set_Type(box_type);
        box_item = static_cast<cMovingSprite*>(mushroom);
    }
    else if (box_type == TYPE_STAR) {
        pAudio->Play_Sound("sprout_1.ogg");
        cjStar* star = new cjStar(m_sprite_manager);
        star->Set_Pos(m_start_pos_x - ((m_item_image->m_w - m_rect.m_w) / 2), m_start_pos_y, 1);
        star->Set_On_Top(this);
        star->Set_Spawned(1);
        // add to global objects
        m_sprite_manager->Add(star);
    }
    else if (box_type == TYPE_GOLDPIECE) {
        pAudio->Play_Sound("item/jewel_1.ogg");

        cJGoldpiece* goldpiece = new cJGoldpiece(m_sprite_manager);
        goldpiece->Set_Pos(m_start_pos_x - ((m_item_image->m_w - m_rect.m_w) / 2), m_pos_y, 1);
        goldpiece->Set_Gold_Color(m_gold_color);
        // add to global objects
        m_sprite_manager->Add(goldpiece);
    }
    else {
        printf("Error : Unknown bonusbox item type %d\n", box_type);
        return;
    }

    // set back random state
    if (random) {
        box_type = TYPE_POWERUP;
    }

    if (box_item) {
        // set posz behind box
        box_item->m_pos_z = m_pos_z - 0.000001f;
        // set spawned
        box_item->Set_Spawned(1);

        // add to item list
        m_active_items.push_back(box_item);
        Update_Valid_Update();
    }
}

void cBonusBox::Update(void)
{
    if (!m_valid_update || !Is_In_Range()) {
        return;
    }

    // update active items
    for (MovingSpriteList::iterator itr = m_active_items.begin(); itr != m_active_items.end();) {
        cPowerUp* powerup = static_cast<cPowerUp*>(*itr);

        if (!powerup->m_active) {
            ++itr;
            continue;
        }

        float diff = powerup->m_pos_y - (m_pos_y - powerup->m_col_rect.m_h - powerup->m_col_pos.m_y);

        // position over the box reached
        if (diff < 0.0f) {
            // clear animation counter
            powerup->m_counter = 0.0f;

            // set powerup default posz
            powerup->m_pos_z = 0.05f;

            // set the item on top
            powerup->Set_On_Top(this, 0);
            // add the item to the level objects
            m_sprite_manager->Add(powerup);

            // remove from array
            itr = m_active_items.erase(itr);
            Update_Valid_Update();
        }
        // move upwards
        else {
            powerup->m_counter += pFramerate->m_speed_factor;
            powerup->Move(0.0f, -1.0f - (diff * 0.1f));

            ++itr;
        }
    }

    cBaseBox::Update();
}

void cBonusBox::Draw(cSurface_Request* request /* = NULL */)
{
    if (!m_valid_draw) {
        return;
    }

    for (MovingSpriteList::iterator itr = m_active_items.begin(); itr != m_active_items.end(); ++itr) {
        cMovingSprite* obj = (*itr);

        if (!obj->m_active) {
            continue;
        }

        obj->Draw();
    }

    cBaseBox::Draw(request);
}

bool cBonusBox::Is_Update_Valid()
{
    // if item is in animation
    if (!m_active_items.empty()) {
        return 1;
    }

    return cBaseBox::Is_Update_Valid();
}

void cBonusBox::Editor_Activate(void)
{
    // BaseBox Settings first
    cBaseBox::Editor_Activate();

    // get window manager
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // Animation
    CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "editor_bonusbox_animation"));
    Editor_Add(UTF8_("Animation"), UTF8_("Use the Power animation if the box has a good or needed item for this level"), combobox, 120, 100);

    combobox->addItem(new CEGUI::ListboxTextItem("Default"));
    combobox->addItem(new CEGUI::ListboxTextItem("Bonus"));
    combobox->addItem(new CEGUI::ListboxTextItem("Power"));

    combobox->setText(reinterpret_cast<const CEGUI::utf8*>(m_anim_type.c_str()));

    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cBonusBox::Editor_Animation_Select, this));

    // Item
    combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "editor_bonusbox_item"));
    Editor_Add(UTF8_("Item"), UTF8_("The item that gets spawned"), combobox, 160, 140);

    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Empty")));
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Random")));
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Mushroom")));
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Fireberry")));
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Mushroom Blue")));
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Mushroom Ghost")));
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Mushroom 1-UP")));
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Star")));
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Goldpiece")));
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Mushroom Poison")));

    if (box_type == TYPE_MUSHROOM_DEFAULT) {
        combobox->setText(UTF8_("Mushroom"));
    }
    else if (box_type == TYPE_FIREPLANT) {
        combobox->setText(UTF8_("Fireberry"));
    }
    else if (box_type == TYPE_MUSHROOM_BLUE) {
        combobox->setText(UTF8_("Mushroom Blue"));
    }
    else if (box_type == TYPE_MUSHROOM_GHOST) {
        combobox->setText(UTF8_("Mushroom Ghost"));
    }
    else if (box_type == TYPE_MUSHROOM_LIVE_1) {
        combobox->setText(UTF8_("Mushroom 1-UP"));
    }
    else if (box_type == TYPE_STAR) {
        combobox->setText(UTF8_("Star"));
    }
    else if (box_type == TYPE_GOLDPIECE) {
        combobox->setText(UTF8_("Goldpiece"));
    }
    else if (box_type == TYPE_MUSHROOM_POISON) {
        combobox->setText(UTF8_("Mushroom Poison"));
    }
    else if (box_type == TYPE_POWERUP) {
        combobox->setText(UTF8_("Random"));
    }
    else {
        combobox->setText(UTF8_("Empty"));
    }

    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cBonusBox::Editor_Item_Select, this));

    // Force best item
    combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "editor_bonusbox_force_best_item"));
    Editor_Add(UTF8_("Force item"), UTF8_("Force best available item when activated"), combobox, 120, 80);

    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Enabled")));
    combobox->addItem(new CEGUI::ListboxTextItem(UTF8_("Disabled")));

    if (m_force_best_item) {
        combobox->setText(UTF8_("Enabled"));
    }
    else {
        combobox->setText(UTF8_("Disabled"));
    }

    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cBonusBox::Editor_Force_best_item_Select, this));

    // gold color
    combobox = static_cast<CEGUI::Combobox*>(wmgr.createWindow("TaharezLook/Combobox", "editor_bonusbox_gold_color"));
    Editor_Add(UTF8_("Gold color"), UTF8_("Gold color if the item is a goldpiece"), combobox, 100, 80);

    combobox->addItem(new CEGUI::ListboxTextItem("yellow"));
    combobox->addItem(new CEGUI::ListboxTextItem("red"));
    combobox->setText(Get_Color_Name(m_gold_color));

    combobox->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::Event::Subscriber(&cBonusBox::Editor_Gold_Color_Select, this));

    // init
    Editor_Init();
}

void cBonusBox::Editor_State_Update(void)
{
    CEGUI::WindowManager& wmgr = CEGUI::WindowManager::getSingleton();

    // Force best item
    CEGUI::Combobox* combobox = static_cast<CEGUI::Combobox*>(wmgr.getWindow("editor_bonusbox_force_best_item"));

    if (box_type == TYPE_UNDEFINED || box_type == TYPE_POWERUP || box_type == TYPE_MUSHROOM_DEFAULT || box_type == TYPE_MUSHROOM_LIVE_1 || box_type == TYPE_MUSHROOM_POISON ||
            box_type == TYPE_MUSHROOM_GHOST || box_type == TYPE_STAR || box_type == TYPE_GOLDPIECE) {
        combobox->setEnabled(0);
    }
    else {
        combobox->setEnabled(1);
    }

    // gold color
    combobox = static_cast<CEGUI::Combobox*>(wmgr.getWindow("editor_bonusbox_gold_color"));

    if (box_type != TYPE_GOLDPIECE) {
        combobox->setEnabled(0);
    }
    else {
        combobox->setEnabled(1);
    }
}

bool cBonusBox::Editor_Animation_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    Set_Animation_Type(item->getText().c_str());

    return 1;
}

bool cBonusBox::Editor_Item_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    if (item->getText().compare(UTF8_("Mushroom")) == 0) {
        Set_Bonus_Type(TYPE_MUSHROOM_DEFAULT);
    }
    else if (item->getText().compare(UTF8_("Fireberry")) == 0) {
        Set_Bonus_Type(TYPE_FIREPLANT);
    }
    else if (item->getText().compare(UTF8_("Mushroom Blue")) == 0) {
        Set_Bonus_Type(TYPE_MUSHROOM_BLUE);
    }
    else if (item->getText().compare(UTF8_("Mushroom Ghost")) == 0) {
        Set_Bonus_Type(TYPE_MUSHROOM_GHOST);
    }
    else if (item->getText().compare(UTF8_("Mushroom 1-UP")) == 0) {
        Set_Bonus_Type(TYPE_MUSHROOM_LIVE_1);
    }
    else if (item->getText().compare(UTF8_("Star")) == 0) {
        Set_Bonus_Type(TYPE_STAR);
    }
    else if (item->getText().compare(UTF8_("Goldpiece")) == 0) {
        Set_Bonus_Type(TYPE_GOLDPIECE);
    }
    else if (item->getText().compare(UTF8_("Mushroom Poison")) == 0) {
        Set_Bonus_Type(TYPE_MUSHROOM_POISON);
    }
    else if (item->getText().compare(UTF8_("Random")) == 0) {
        Set_Bonus_Type(TYPE_POWERUP);
    }
    else {
        Set_Bonus_Type(TYPE_UNDEFINED);
    }

    Editor_State_Update();

    return 1;
}

bool cBonusBox::Editor_Force_best_item_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    if (item->getText().compare(UTF8_("Enabled")) == 0) {
        Set_Force_Best_Item(1);
    }
    else {
        Set_Force_Best_Item(0);
    }

    return 1;
}

bool cBonusBox::Editor_Gold_Color_Select(const CEGUI::EventArgs& event)
{
    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(event);
    CEGUI::ListboxItem* item = static_cast<CEGUI::Combobox*>(windowEventArgs.window)->getSelectedItem();

    Set_Goldcolor(Get_Color_Id(item->getText().c_str()));

    return 1;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC
