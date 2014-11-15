#include "errors.hpp"

using namespace TSC;

TSCError::TSCError()
{
    m_tsc_errmsg = "Unknown TSC exception.";
}


TSCError::TSCError(std::string message)
{
    m_tsc_errmsg = message;
}

TSCError::~TSCError() throw()
{
    //
}

const char* TSCError::what() const throw()
{
    return m_tsc_errmsg.c_str();
}

ConfigurationError::ConfigurationError(std::string msg)
    : TSCError(msg)
{
    //
}

ConfigurationError::~ConfigurationError() throw()
{
    //
}

XmlKeyDoesNotExist::XmlKeyDoesNotExist(std::string key)
{
    m_key = key;
}

XmlKeyDoesNotExist::~XmlKeyDoesNotExist() throw()
{
    //
}

std::string XmlKeyDoesNotExist::Get_Key()
{
    return m_key;
}

const char* XmlKeyDoesNotExist::what() const throw()
{
    std::string err = "XML key '" + m_key + "' does not exist!\n";
    return err.c_str();
}

NotImplementedError::NotImplementedError(std::string message)
    : TSCError(message)
{
}

NotImplementedError::~NotImplementedError() throw()
{
    //
}

InvalidLevelError::InvalidLevelError(std::string message)
    : TSCError(message)
{
    //
}

InvalidLevelError::~InvalidLevelError() throw()
{
    //
}

InvalidSavegameError::InvalidSavegameError(unsigned int slot, std::string message)
    : TSCError(message)
{
    m_slot = slot;
}

InvalidSavegameError::~InvalidSavegameError() throw()
{
    //
}

RestartedXmlParserError::RestartedXmlParserError()
{
    //
}

RestartedXmlParserError::~RestartedXmlParserError() throw()
{
    //
}

const char* RestartedXmlParserError::what() const throw()
{
    return "Restarted a one-time XML parser!";
}

InvalidMovingStateError::InvalidMovingStateError(Moving_state state)
{
    m_state = state;
}

InvalidMovingStateError::~InvalidMovingStateError() throw()
{
    //
}

const char* InvalidMovingStateError::what() const throw()
{
    std::stringstream ss;
    ss << "Invalid moving state '" << m_state << "' for this object!\n";
    return ss.str().c_str();
}

EditorError::EditorError(std::string msg)
    : TSCError(msg)
{
    //
}

EditorError::~EditorError() throw()
{
    //
}

EditorSpriteCopyFailedError::EditorSpriteCopyFailedError(cSprite* p_sprite)
    : EditorError(std::string("Editor sprite '") + p_sprite->Create_Name() + "' copy failed!")
{
    mp_sprite = p_sprite;
}

EditorSpriteCopyFailedError::~EditorSpriteCopyFailedError() throw()
{
    //
}

cSprite* EditorSpriteCopyFailedError::Get_Sprite()
{
    return mp_sprite;
}
