/***************************************************************************
 * errors.hpp - Exceptions used in TSC
 *
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

#ifndef TSC_ERRORS_HPP
#define TSC_ERRORS_HPP
#include "global_game.hpp"
#include "../objects/movingsprite.hpp"

namespace TSC {

    /**
     * Base class of all TSC-related exceptions.
     */
    class TSCError: public std::exception {
    public:
        TSCError();
        TSCError(std::string message);
        virtual ~TSCError() throw();

        virtual const char* what() const throw();
    protected:
        std::string m_tsc_errmsg;
    };

    class ConfigurationError: public TSCError {
    public:
        ConfigurationError(std::string message);
        virtual ~ConfigurationError() throw();
    };

    /**
     * This exception is thrown when an expected XML
     * key is not found.
     */
    class XmlKeyDoesNotExist: public TSCError {
    public:
        XmlKeyDoesNotExist(std::string key);
        virtual ~XmlKeyDoesNotExist() throw();

        virtual const char* what() const throw();
        std::string Get_Key();
    protected:
        std::string m_key;
    };

    // Thrown if specific things are not implemented for
    // some reason.
    class NotImplementedError: public TSCError {
    public:
        NotImplementedError(std::string message);
        virtual ~NotImplementedError() throw();
    };

    class InvalidLevelError: public TSCError {
    public:
        InvalidLevelError(std::string message);
        virtual ~InvalidLevelError() throw();
        virtual const char* what() const throw();
    };

    class InvalidSavegameError: public TSCError {
    public:
        InvalidSavegameError(unsigned int slot, std::string message);
        virtual ~InvalidSavegameError() throw();
        inline unsigned int Get_Slot(){return m_slot;}
    private:
        unsigned int m_slot;
    };

    class RestartedXmlParserError: public TSCError {
    public:
        RestartedXmlParserError();
        virtual ~RestartedXmlParserError() throw();
        virtual const char* what() const throw();
    };

    class InvalidMovingStateError: public TSCError {
    public:
        InvalidMovingStateError(Moving_state state);
        virtual ~InvalidMovingStateError() throw();
        virtual const char* what() const throw();
    protected:
        Moving_state m_state;
    };

    class EditorError: public TSCError {
    public:
        EditorError(std::string msg);
        virtual ~EditorError() throw();
    };

    class EditorSpriteCopyFailedError: public EditorError {
    public:
        EditorSpriteCopyFailedError(cSprite* p_sprite);
        virtual ~EditorSpriteCopyFailedError() throw();
        cSprite* Get_Sprite();
    protected:
        cSprite* mp_sprite;
    };

}

#endif
