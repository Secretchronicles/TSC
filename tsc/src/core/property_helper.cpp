/***************************************************************************
 * property_helper.cpp  -  property helper
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

#include "../core/property_helper.hpp"
#include "../core/i18n.hpp"
#include "../video/color.hpp"
#include "../objects/sprite.hpp"

namespace TSC {

void string_replace_all(std::string& str, const std::string& search, const std::string& format)
{
    size_t pos = 0;

    while ((pos = str.find(search, pos)) != std::string::npos) {
        str.replace(pos, search.length(), format);
        pos += format.length();
    }
}

void cegui_string_replace_all(CEGUI::String& str, const CEGUI::String& search, const CEGUI::String& format)
{
    size_t pos = 0;

    while ((pos = str.find(search, pos)) != CEGUI::String::npos) {
        str.replace(pos, search.length(), format);
        pos += format.length();
    }
}

std::string string_trim_from_end(std::string str, const char search)
{
    // find last position from end which is not the given character
    size_t pos = str.find_last_not_of(search);

    // if all match or empty
    if (pos == std::string::npos) {
        return std::string();
    }
    else {
        return str.substr(0, pos + 1);
    }
}

std::string int_to_string(const int number)
{
    std::ostringstream os;
    os << number;
    return os.str();
}

std::string uint_to_string(const unsigned int number)
{
    std::ostringstream os;
    os << number;
    return os.str();
}

std::string int64_to_string(const Uint64 number)
{
    std::ostringstream os;
    os << number;
    return os.str();
}

std::string long_to_string(const long number)
{
    std::ostringstream os;
    os << number;
    return os.str();
}

std::string bool_to_string(const bool val)
{
    return val ? "1" : "0";
}

// from stringencoders for float_to_string
/**
 * Powers of 10
 * 10^0 to 10^9
 */
static const double pow_of_10[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

// function from stringencoders for float_to_string
static void strreverse(char* begin, char* end)
{
    char aux;
    while (end > begin)
        aux = *end, *end-- = *begin, *begin++ = aux;
}

/* function from stringencoders 3.10.3 with modifications. Copyright (C) 2007 Nick Galbreath -- nickg [at] modp [dot] com
 * BSD License - http://www.opensource.org/licenses/bsd-license.php
 */
std::string float_to_string(double value, int prec /* = 6 */, bool keep_zeros /* = 1 */)
{
    /* Hacky test for NaN
     * under -fast-math this won't work, but then you also won't
     * have correct nan values anyways.  The alternative is
     * to link with libmath (bad) or hack IEEE double bits (bad)
     */
    if (!(value == value)) {
        return "nan";
    }

    /* if input is larger than thres_max, revert to native */
    const double thres_max = static_cast<double>(0x7FFFFFFF);

    double diff = 0.0;
    char str[64];
    char* wstr = str;

    if (prec < 0) {
        prec = 0;
    }
    else if (prec > 6) {
        /* precision of >= 7 for float can lead to overflow errors */
        prec = 6;
    }

    /* we'll work in positive values and deal with the
       negative sign issue later */
    int neg = 0;
    if (value < 0) {
        neg = 1;
        value = -value;
    }

    int whole = static_cast<int>(value);
    double tmp = (value - whole) * pow_of_10[prec];
    uint32_t frac = static_cast<uint32_t>(tmp);
    diff = tmp - frac;

    if (diff > 0.5) {
        ++frac;
        /* handle rollover, e.g.  case 0.99 with prec 1 is 1.0  */
        if (frac >= pow_of_10[prec]) {
            frac = 0;
            ++whole;
        }
    }
    else if (diff == 0.5 && ((frac == 0) || (frac & 1))) {
        /* if halfway, round up if odd, OR
           if last digit is 0.  That last part is strange */
        ++frac;
    }

    /* for very large numbers switch back to native for exponentials. */
    /*
        normal printf behavior is to print EVERY whole number digit
        which can be 100s of characters overflowing your buffers == bad
    */
    if (value > thres_max) {
        std::ostringstream temp;
        temp.setf(std::ios_base::fixed);
        temp << value;

        return temp.str();
    }

    if (prec == 0) {
        diff = value - whole;
        if (diff > 0.5) {
            /* greater than 0.5, round up, e.g. 1.6 -> 2 */
            ++whole;
        }
        else if (diff == 0.5 && (whole & 1)) {
            /* exactly 0.5 and ODD, then round up */
            /* 1.5 -> 2, but 2.5 -> 2 */
            ++whole;
        }
    }
    else {
        int count = prec;

        if (!keep_zeros) {
            if (frac) {
                // now do fractional part, as an unsigned number
                // we know it is not 0 but we can have leading zeros, these
                // should be removed
                while (!(frac % 10)) {
                    --count;
                    frac /= 10;
                }

                // now do fractional part, as an unsigned number
                do {
                    --count;
                    *wstr++ = (char)(48 + (frac % 10));
                }
                while (frac /= 10);
                // add extra 0s
                while (count-- > 0) *wstr++ = '0';
                // add decimal
                *wstr++ = '.';
            }
        }
        else {
            // now do fractional part, as an unsigned number
            do {
                --count;
                *wstr++ = (char)(48 + (frac % 10));
            }
            while (frac /= 10);
            // add extra 0s
            while (count-- > 0) *wstr++ = '0';
            // add decimal
            *wstr++ = '.';
        }
    }

    // do whole part
    // Take care of sign
    // Conversion. Number is reversed.
    do* wstr++ = (char)(48 + (whole % 10));
    while (whole /= 10);
    if (neg) {
        *wstr++ = '-';
    }

    *wstr='\0';
    strreverse(str, wstr - 1);

    return str;
}

// string conversion helper
template <class T> bool from_string(T& t, const std::string& s, std::ios_base &(*f)(std::ios_base&))
{
    std::istringstream iss(s);
    return !(iss >> f >> t).fail();
}

int string_to_int(const std::string& str)
{
    int num = 0;
    // use helper
    from_string<int>(num, str, std::dec);
    return num;
}

unsigned int string_to_uint(const std::string& str)
{
    unsigned int num = 0;
    // use helper
    from_string<unsigned int>(num, str, std::dec);
    return num;
}

Uint64 string_to_int64(const std::string& str)
{
    Uint64 num = 0;
    // use helper
    from_string<Uint64>(num, str, std::dec);
    return num;
}

long string_to_long(const std::string& str)
{
    long num = 0;
    // use helper
    from_string<long>(num, str, std::dec);
    return num;
}

float string_to_float(const std::string& str)
{
    float num = 0.0f;
    // use helper
    from_string<float>(num, str, std::dec);
    return num;
}

double string_to_double(const std::string& str)
{
    double num = 0.0;
    // use helper
    from_string<double>(num, str, std::dec);
    return num;
}

bool string_to_bool(const std::string& str)
{
    if (str == "0")
        return false;
    else
        return true;
}

unsigned int string_to_version_number(std::string str)
{
    if (str.empty()) {
        return 0;
    }

    std::string::size_type pos = str.find('.');

    // only major version
    if (pos == std::string::npos) {
        return string_to_int(str) * 10000;
    }

    unsigned int ver = 0;

    if (pos != 0) {
        // number before first '.' is major
        ver = string_to_int(str.substr(0, pos)) * 10000;
    }

    str.erase(0, pos + 1);
    pos = str.find('.');

    if (pos != 0) {
        // number after first '.' is minor
        ver += string_to_int(str.substr(0, pos)) * 100;
    }

    if (pos == std::string::npos) {
        return ver;
    }

    str.erase(0, pos + 1);

    if (!str.empty()) {
        // number after second '.' is patch
        ver += string_to_int(str);
    }

    return ver;
}

std::string xml_string_to_string(std::string str)
{
    while (1) {
        std::string::size_type pos = str.find("<br/>");

        if (pos == std::string::npos) {
            break;
        }

        str.replace(pos, 5, "\n");
    }

    return str;
}

#ifdef _WIN32
std::string ucs2_to_utf8(const std::wstring& utf16)
{
    if (utf16.empty()) {
        return std::string();
    }

    const int utf8_length = WideCharToMultiByte(CP_UTF8, 0, utf16.data(), utf16.length(), NULL, 0, NULL, NULL);

    if (utf8_length == 0) {
        printf("Warning: ucs2_to_utf8 : WideCharToMultiByte returned zero length");
        return std::string();
    }

    std::string utf8(utf8_length, 0);

    if (!WideCharToMultiByte(CP_UTF8, 0 , utf16.data(), utf16.length(), &utf8[0], utf8.length(), NULL, NULL)) {
        printf("Warning: ucs2_to_utf8 : WideCharToMultiByte conversion failed");
    }

    return utf8;
}

std::wstring utf8_to_ucs2(const std::string& utf8)
{
    if (utf8.empty()) {
        return std::wstring();
    }

    const int utf16_length = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), utf8.length(), NULL, 0);

    if (utf16_length == 0) {
        printf("Warning: utf8_to_ucs2 : MultiByteToWideChar returned zero length");
        return std::wstring();
    }

    std::wstring utf16(utf16_length, 0);
    if (!MultiByteToWideChar(CP_UTF8, 0, utf8.data(), utf8.length(), &utf16[0], utf16.length())) {
        printf("Warning: utf8_to_ucs2 : MultiByteToWideChar conversion failed");
    }

    return utf16;
}
#endif

/**
 * For Windows, we have to use wide strings, otherwise the files will
 * have gibberish and incorrect names. However, the standard C++ library
 * does not provide a way to pass pathes as wide strings, and using normal
 * UTF-8-encoded strings, as said, will cause incorrect filenames. MSVC offers a
 * nonstandard extension to the C++ standard library that allows to pass wide strings
 * to STL functions, but the MinGW project’s g++ doesn’t support this MS-specificum.
 * However, boost::filesystem::path, a class for managing pathes, supports wide
 * strings on Windows and normal strings on *nix, hence we can use this a
 * uniform interface to the filesystem. Note that boost::filesystem::path however
 * does *not* itself carry out the conversion of a normal UTF-8 string to a wide
 * UTF-16 string, so we do this ourselves by using the Windows API directly in the
 * utf8_to_ucs2() function, which is, if used correctly, independent from the
 * system locale and returns a proper UTF-16LE string. It just carries round UCS-2
 * (the predecissor of UTF-16) in its name due to historical reasons.
 *
 * The utf8_to_path() function is defined differently according to the platform,
 * allowing us to map platform-specific conversion to the uniform interface
 * of a boost::filesystem::path instance. The same goes vice-versa for the
 * path_to_utf8() function.
 */
#ifdef _WIN32

boost::filesystem::path utf8_to_path(const std::string& utf8)
{
    return boost::filesystem::path(utf8_to_ucs2(utf8));
}

std::string path_to_utf8(const boost::filesystem::path& path)
{
    return ucs2_to_utf8(path.native());
}

#else

boost::filesystem::path utf8_to_path(const std::string& utf8)
{
    return boost::filesystem::path(utf8);
}

std::string path_to_utf8(const boost::filesystem::path& path)
{
    return std::string(path.native()); // Copy
}

#endif

std::string Time_to_String(time_t t, const char* format)
{
    char str_time[60];
    strftime(str_time, 60, format, localtime(&t));

    return str_time;
}

ObjectDirection Get_Opposite_Direction(const ObjectDirection direction)
{
    switch (direction) {
    case DIR_UP:
        return DIR_DOWN;
    case DIR_DOWN:
        return DIR_UP;
    case DIR_LEFT:
        return DIR_RIGHT;
    case DIR_RIGHT:
        return DIR_LEFT;
    case DIR_HORIZONTAL:
        return DIR_VERTICAL;
    case DIR_VERTICAL:
        return DIR_HORIZONTAL;
    default:
        break;
    }

    return DIR_UNDEFINED;
}

std::string Get_Direction_Name(const ObjectDirection dir)
{
    switch (dir) {
    case DIR_UNDEFINED:
        return N_("undefined");
    case DIR_LEFT:
        return N_("left");
    case DIR_RIGHT:
        return N_("right");
    case DIR_UP:
        return N_("up");
    case DIR_DOWN:
        return N_("down");
    case DIR_TOP_LEFT:
        return N_("top_left");
    case DIR_TOP_RIGHT:
        return N_("top_right");
    case DIR_BOTTOM_LEFT:
        return N_("bottom_left");
    case DIR_BOTTOM_RIGHT:
        return N_("bottom_right");
    case DIR_LEFT_TOP:
        return N_("left_top");
    case DIR_LEFT_BOTTOM:
        return N_("left_bottom");
    case DIR_RIGHT_TOP:
        return N_("right_top");
    case DIR_RIGHT_BOTTOM:
        return N_("right_bottom");
    case DIR_HORIZONTAL:
        return N_("horizontal");
    case DIR_VERTICAL:
        return N_("vertical");
    case DIR_ALL:
        return N_("all");
    case DIR_FIRST:
        return N_("first");
    case DIR_LAST:
        return N_("last");
    default:
        break;
    }

    return "";
}

ObjectDirection Get_Direction_Id(const std::string& str_direction)
{
    if (str_direction.compare("undefined") == 0) {
        return DIR_UNDEFINED;
    }
    else if (str_direction.compare("left") == 0) {
        return DIR_LEFT;
    }
    else if (str_direction.compare("right") == 0) {
        return DIR_RIGHT;
    }
    else if (str_direction.compare("up") == 0) {
        return DIR_UP;
    }
    else if (str_direction.compare("down") == 0) {
        return DIR_DOWN;
    }
    else if (str_direction.compare("top_left") == 0) {
        return DIR_TOP_LEFT;
    }
    else if (str_direction.compare("top_right") == 0) {
        return DIR_TOP_RIGHT;
    }
    else if (str_direction.compare("bottom_left") == 0) {
        return DIR_BOTTOM_LEFT;
    }
    else if (str_direction.compare("bottom_right") == 0) {
        return DIR_BOTTOM_RIGHT;
    }
    else if (str_direction.compare("left_top") == 0) {
        return DIR_LEFT_TOP;
    }
    else if (str_direction.compare("left_bottom") == 0) {
        return DIR_LEFT_BOTTOM;
    }
    else if (str_direction.compare("right_top") == 0) {
        return DIR_RIGHT_TOP;
    }
    else if (str_direction.compare("right_bottom") == 0) {
        return DIR_RIGHT_BOTTOM;
    }
    else if (str_direction.compare("horizontal") == 0) {
        return DIR_HORIZONTAL;
    }
    else if (str_direction.compare("vertical") == 0) {
        return DIR_VERTICAL;
    }
    else if (str_direction.compare("all") == 0) {
        return DIR_ALL;
    }
    else if (str_direction.compare("first") == 0) {
        return DIR_FIRST;
    }
    else if (str_direction.compare("last") == 0) {
        return DIR_LAST;
    }

    return DIR_UNDEFINED;
}

Color Get_Sprite_Color(const cSprite* sprite)
{
    switch (sprite->m_sprite_array) {
    case ARRAY_ENEMY: {
        return red;
    }
    case ARRAY_ACTIVE: {
        return blue;
    }
    case ARRAY_MASSIVE: {
        if (sprite->m_type == TYPE_PLAYER) {
            return lila;
        }

        return orange;
    }
    case ARRAY_PASSIVE: {
        if (sprite->m_massive_type == MASS_FRONT_PASSIVE) {
            return greenyellow;
        }

        return green;
    }
    case ARRAY_HUD: {
        return blackalpha128;
    }
    case ARRAY_LAVA: {
        return red;
    }
    default: {
        break;
    }
    }

    return lightgreyalpha64;
}

std::string Get_Massive_Type_Name(const MassiveType mtype)
{
    switch (mtype) {
    case MASS_PASSIVE:
        return "passive";
    case MASS_MASSIVE:
        return "massive";
    case MASS_HALFMASSIVE:
        return "halfmassive";
    case MASS_CLIMBABLE:
        return "climbable";
    case MASS_FRONT_PASSIVE:
        return "front_passive";
    default:
        break;
    }

    return "";
}

MassiveType Get_Massive_Type_Id(const std::string& str_massivetype)
{
    if (str_massivetype.compare("passive") == 0) {
        return MASS_PASSIVE;
    }
    else if (str_massivetype.compare("front_passive") == 0) {
        return MASS_FRONT_PASSIVE;
    }
    else if (str_massivetype.compare("massive") == 0) {
        return MASS_MASSIVE;
    }
    else if (str_massivetype.compare("halfmassive") == 0) {
        return MASS_HALFMASSIVE;
    }
    else if (str_massivetype.compare("climbable") == 0) {
        return MASS_CLIMBABLE;
    }

    return MASS_PASSIVE;
}

Color Get_Massive_Type_Color(MassiveType mtype)
{
    switch (mtype) {
    case MASS_MASSIVE:
        return lightred;
    case MASS_HALFMASSIVE:
        return orange;
    case MASS_PASSIVE:
        return lightgreen;
    case MASS_CLIMBABLE:
        return lila;
    case MASS_FRONT_PASSIVE:
        return greenyellow;
    default:
        break;
    }

    return white;
}

std::string Get_Ground_Type_Name(const GroundType gtype)
{
    switch (gtype) {
    case GROUND_NORMAL:
        return "normal";
    case GROUND_EARTH:
        return "earth";
    case GROUND_ICE:
        return "ice";
    case GROUND_SAND:
        return "sand";
    case GROUND_STONE:
        return "stone";
    case GROUND_PLASTIC:
        return "plastic";
    default:
        break;
    }

    return "";
}

GroundType Get_Ground_Type_Id(const std::string& str_groundtype)
{
    if (str_groundtype.compare("normal") == 0) {
        return GROUND_NORMAL;
    }
    else if (str_groundtype.compare("earth") == 0) {
        return GROUND_EARTH;
    }
    else if (str_groundtype.compare("ice") == 0) {
        return GROUND_ICE;
    }
    else if (str_groundtype.compare("sand") == 0) {
        return GROUND_SAND;
    }
    else if (str_groundtype.compare("stone") == 0) {
        return GROUND_STONE;
    }
    else if (str_groundtype.compare("plastic") == 0) {
        return GROUND_PLASTIC;
    }

    return GROUND_NORMAL;
}

std::string Get_Level_Land_Type_Name(const LevelLandType land_type)
{
    switch (land_type) {
    case LLT_UNDEFINED:
        return N_("undefined");
    case LLT_GREEN:
        return N_("green");
    case LLT_JUNGLE:
        return N_("jungle");
    case LLT_ICE:
        return N_("ice");
    case LLT_SNOW:
        return N_("snow");
    case LLT_WATER:
        return N_("water");
    case LLT_CANDY:
        return N_("candy");
    case LLT_DESERT:
        return N_("desert");
    case LLT_SAND:
        return N_("sand");
    case LLT_CASTLE:
        return N_("castle");
    case LLT_UNDERGROUND:
        return N_("underground");
    case LLT_CRYSTAL:
        return N_("crystal");
    case LLT_GHOST:
        return N_("ghost");
    case LLT_MUSHROOM:
        return N_("mushroom");
    case LLT_SKY:
        return N_("sky");
    case LLT_PLASTIC:
        return N_("plastic");
    default:
        break;
    }

    return "";
}

LevelLandType Get_Level_Land_Type_Id(const std::string& str_type)
{
    if (str_type.compare("undefined") == 0) {
        return LLT_UNDEFINED;
    }
    else if (str_type.compare("green") == 0) {
        return LLT_GREEN;
    }
    else if (str_type.compare("jungle") == 0) {
        return LLT_JUNGLE;
    }
    else if (str_type.compare("ice") == 0) {
        return LLT_ICE;
    }
    else if (str_type.compare("snow") == 0) {
        return LLT_SNOW;
    }
    else if (str_type.compare("water") == 0) {
        return LLT_WATER;
    }
    else if (str_type.compare("candy") == 0) {
        return LLT_CANDY;
    }
    else if (str_type.compare("desert") == 0) {
        return LLT_DESERT;
    }
    else if (str_type.compare("sand") == 0) {
        return LLT_SAND;
    }
    else if (str_type.compare("castle") == 0) {
        return LLT_CASTLE;
    }
    else if (str_type.compare("underground") == 0) {
        return LLT_UNDERGROUND;
    }
    else if (str_type.compare("crystal") == 0) {
        return LLT_CRYSTAL;
    }
    else if (str_type.compare("ghost") == 0) {
        return LLT_GHOST;
    }
    else if (str_type.compare("mushroom") == 0) {
        return LLT_MUSHROOM;
    }
    else if (str_type.compare("sky") == 0) {
        return LLT_SKY;
    }
    else if (str_type.compare("plastic") == 0) {
        return LLT_PLASTIC;
    }

    return LLT_UNDEFINED;
}

std::string Get_Color_Name(const DefaultColor color)
{
    switch (color) {
    case COL_DEFAULT:
        return N_("default");
    case COL_WHITE:
        return N_("white");
    case COL_BLACK:
        return N_("black");
    case COL_RED:
        return N_("red");
    case COL_ORANGE:
        return N_("orange");
    case COL_YELLOW:
        return N_("yellow");
    case COL_GREEN:
        return N_("green");
    case COL_BLUE:
        return N_("blue");
    case COL_BROWN:
        return N_("brown");
    case COL_GREY:
        return N_("grey");
    case COL_VIOLET:
        return N_("violet");
    default:
        break;
    }

    return "";
}

DefaultColor Get_Color_Id(const std::string& str_color)
{
    if (str_color.compare("white") == 0) {
        return COL_WHITE;
    }
    else if (str_color.compare("black") == 0) {
        return COL_BLACK;
    }
    else if (str_color.compare("red") == 0) {
        return COL_RED;
    }
    else if (str_color.compare("orange") == 0) {
        return COL_ORANGE;
    }
    else if (str_color.compare("yellow") == 0) {
        return COL_YELLOW;
    }
    else if (str_color.compare("green") == 0) {
        return COL_GREEN;
    }
    else if (str_color.compare("blue") == 0) {
        return COL_BLUE;
    }
    else if (str_color.compare("brown") == 0) {
        return COL_BROWN;
    }
    else if (str_color.compare("grey") == 0) {
        return COL_GREY;
    }
    else if (str_color.compare("violet") == 0) {
        return COL_VIOLET;
    }

    return COL_DEFAULT;
}

std::string Get_Difficulty_Name(Uint8 difficulty)
{
    if (difficulty == 0) {
        return "Undefined";
    }
    else if (difficulty < 10) {
        return "Easy-Peasy";
    }
    else if (difficulty < 20) {
        return "Very Easy";
    }
    else if (difficulty < 30) {
        return "Easy";
    }
    else if (difficulty < 40) {
        return "Easy-Medium";
    }
    else if (difficulty < 50) {
        return "Medium";
    }
    else if (difficulty < 60) {
        return "Medium-Hard";
    }
    else if (difficulty < 70) {
        return "Hard";
    }
    else if (difficulty < 80) {
        return "Very Hard";
    }
    else if (difficulty < 90) {
        return "Extreme";
    }
    else if (difficulty < 95) {
        return "Nightmare";
    }

    return "Ultimate";
}

std::string readfile(boost::filesystem::ifstream& file)
{
    std::string content;
    std::string line;
    while (!file.eof()) {
        std::getline(file, line);
        content.append(line);
        content.append("\n");
    }

    return content;
}

/* *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

} // namespace TSC
