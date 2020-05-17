/***************************************************************************
 * color.h
 *
 * Copyright © 2005 - 2011 Florian Richter
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

#ifndef TSC_COLOR_HPP
#define TSC_COLOR_HPP

namespace TSC {

    /* *** *** *** *** *** *** *** *** Color class *** *** *** *** *** *** *** *** *** */

    /**
     * Class representing a color in TSC's system. It provides converters to
     * SFML and CEGUI colors.
     *
     * The color components are stored as an integer of range 0-255, where 0 denotes
     * to use nothing of this color and 255 to use it completely. Alpha values range
     * from 0 (transparent) to 255 (opaque) as well.
     */
    class Color {
    public:
        Color(void)
        {
            red = 0;
            green = 0;
            blue = 0;
            alpha = 255;
        }

        /**
         * Construct a color from the colorr components, which each may be between 0
         * and 255 (for alpha, 0 means transparent and 255 opaque).
         */
        Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        {
            red = r;
            green = g;
            blue = b;
            alpha = a;
        }

        /**
         * Construct a color from the color components, which each may be between
         * 0 (nothing) and 255 (full) (for alpha, 0 means transparent and 1 means
         * opaque). The float values get converted to integer values internally.
         */
        Color(float r, float g, float b, float a = 1.0f)
        {
            red = static_cast<uint8_t>(r * 255);
            green = static_cast<uint8_t>(g * 255);
            blue = static_cast<uint8_t>(b * 255);
            alpha = static_cast<uint8_t>(a * 255);
        }

        /**
         * Construct a grey tone. 0 means white, 255 means black.
         */
        Color(uint8_t grey)
        {
            red = grey;
            green = grey;
            blue = grey;
            alpha = 255;
        }

        /// Creates an instance from a hex color string of form "AARRGGBB",
        /// where AA refers to the alpha value.
        Color(std::string hexstr)
        {
            alpha = static_cast<uint8_t>(std::stoul(hexstr.substr(0, 2), NULL, 16));
            red   = static_cast<uint8_t>(std::stoul(hexstr.substr(2, 2), NULL, 16));
            green = static_cast<uint8_t>(std::stoul(hexstr.substr(4, 2), NULL, 16));
            blue  = static_cast<uint8_t>(std::stoul(hexstr.substr(6, 2), NULL, 16));
        }

        /// Returns it as SFML color.
        inline sf::Color Get_SFML_Color(void) const
        {
            return sf::Color(red, green, blue);
        }

        /// Returns it as CEGUI color
        inline CEGUI::Colour Get_cegui_Color(void) const
        {
            return CEGUI::Colour(static_cast<float>(red) / 255, static_cast<float>(green) / 255, static_cast<float>(blue) / 255, static_cast<float>(alpha) / 255);
        }

        // Returns it as a CEGUI color string of form "AARRGGBB",
        // where AA refers to the alpha value.
        inline std::string Get_cegui_Colorstr(void) const
        {
            char buf[9]; /* 8+NUL */
            snprintf(buf, 9, "%02x%02x%02x%02x", alpha, red, green, blue);
            return buf;
        }

        // += operator
        inline void operator += (const Color& c)
        {
            red += c.red;
            green += c.green;
            blue += c.blue;
            alpha += c.alpha;
        }

        // -= operator
        inline void operator -= (const Color& c)
        {
            red -= c.red;
            green -= c.green;
            blue -= c.blue;
            alpha -= c.alpha;
        }

        // + operator
        inline Color operator + (const Color& c) const
        {
            return Color(static_cast<uint8_t>(red + c.red), green + c.green, blue + c.blue, alpha + c.alpha);
        }

        // - operator
        inline Color operator - (const Color& c) const
        {
            return Color(static_cast<uint8_t>(red - c.red), green - c.green, blue - c.blue, alpha - c.alpha);
        }

        // assignment operator
        inline Color& operator = (const Color& c)
        {
            red = c.red;
            green = c.green;
            blue = c.blue;
            alpha = c.alpha;

            return *this;
        }

        // compare
        inline bool operator == (const Color& c) const
        {
            return red == c.red && green == c.green && blue == c.blue && alpha == c.alpha;
        }

        inline bool operator == (const sf::Color& c) const
        {
            return red == c.r && green == c.g && blue == c.b && alpha == c.a;
        }

        inline bool operator != (const Color& c) const
        {
            return !(operator == (c));
        }

        inline bool operator != (const sf::Color& c) const
        {
            return !(operator == (c));
        }

        uint8_t red, green, blue, alpha;
    };

// Generic Colors
    static const Color blue = Color(static_cast<uint8_t>(150), 200, 225);
    static const Color darkblue = Color(static_cast<uint8_t>(0), 0, 128);
    static const Color lightblue = Color(static_cast<uint8_t>(41), 167, 255);
    static const Color black = Color(static_cast<uint8_t>(0), 0, 0);
    static const Color blackalpha128 = Color(static_cast<uint8_t>(0), 0, 0, 128);
    static const Color blackalpha192 = Color(static_cast<uint8_t>(0), 0, 0, 192);
    static const Color white = Color(static_cast<uint8_t>(255), 255, 255);
    static const Color whitealpha128 = Color(static_cast<uint8_t>(255), 255, 255, 128);
    static const Color grey = Color(static_cast<uint8_t>(128), 128, 128);
    static const Color lightgrey = Color(static_cast<uint8_t>(64), 64, 64);
    static const Color lightgreyalpha64 = Color(static_cast<uint8_t>(64), 64, 64, 64);
    static const Color green = Color(static_cast<uint8_t>(0), 230, 0);
    static const Color lightgreen = Color(static_cast<uint8_t>(20), 253, 20);
    static const Color lightgreenalpha64 = Color(static_cast<uint8_t>(30), 230, 30, 64);
    static const Color yellow = Color(static_cast<uint8_t>(255), 245, 10);
    static const Color greenyellow = Color(static_cast<uint8_t>(154), 205, 50);
    static const Color darkgreen = Color(static_cast<uint8_t>(1), 119, 34);
    static const Color red = Color(static_cast<uint8_t>(250), 0, 0);
    static const Color lightred = Color(static_cast<uint8_t>(255), 40, 20);
    static const Color lila = Color(static_cast<uint8_t>(200), 0, 255);
    static const Color orange = Color(static_cast<uint8_t>(248), 191, 38);
    static const Color lightorange = Color(static_cast<uint8_t>(255), 220, 100);

    /* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC

#endif
