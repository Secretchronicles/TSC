/***************************************************************************
 * game_console.cpp - The game console activated with F7
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

#include "../core/global_basic.hpp"
#include <mruby/error.h>
#include "config.hpp"
#include "../core/i18n.hpp"
#include "../core/filesystem/resource_manager.hpp"
#include "../level/level.hpp"
#include "game_console.hpp"

// extern
TSC::cGame_Console* TSC::gp_game_console = NULL;

using namespace TSC;
namespace fs = boost::filesystem;

static std::string timestamp()
{
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);

    char buf[128];
    sprintf(buf,
            "%04d-%02d-%02d %02d:%02d:%02d",
            timeinfo->tm_year + 1900,
            timeinfo->tm_mon + 1,
            timeinfo->tm_mday,
            timeinfo->tm_hour,
            timeinfo->tm_min,
            timeinfo->tm_sec);

    return std::string(buf);
}

cGame_Console::cGame_Console()
    : mp_console_root(NULL),
      mp_input_edit(NULL),
      mp_output_edit(NULL),
      mp_lino_text(NULL),
      m_history_idx(0)
{
    // Load layout file and add it to the root
    mp_console_root = CEGUI::WindowManager::getSingleton().loadLayoutFromFile("game_console.layout");
    CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->addChild(mp_console_root);

    mp_console_root->hide(); // Do not show for now

    mp_input_edit  = static_cast<CEGUI::Editbox*>(mp_console_root->getChild("input"));
    mp_output_edit = static_cast<CEGUI::MultiLineEditbox*>(mp_console_root->getChild("output"));
    mp_lino_text   = mp_console_root->getChild("lineno");

    // Terminals usually don't have scrollbars
    mp_output_edit->setShowVertScrollbar(false);

    mp_input_edit->subscribeEvent(CEGUI::Editbox::EventTextAccepted,
                                  CEGUI::Event::Subscriber(&cGame_Console::on_input_accepted, this));
    mp_input_edit->subscribeEvent(CEGUI::Editbox::EventKeyUp,
                                  CEGUI::Event::Subscriber(&cGame_Console::on_key_up, this));

    // Open logfile
    m_logfile.open(pResource_Manager->Get_User_GameConsole_Logfile(),
                   fs::ofstream::out | fs::ofstream::app);
 
    m_logfile << "--- Logfile opened on " << timestamp() << " ---" << std::endl;

    Reset();
}

cGame_Console::~cGame_Console()
{
    if (mp_console_root) {
        CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->removeChild(mp_console_root);
        CEGUI::WindowManager::getSingleton().destroyWindow(mp_console_root);
        mp_console_root = NULL;
    }

    m_logfile << "--- Logfile closed on " << timestamp() << " ---" << std::endl;
    m_logfile.close();
}

void cGame_Console::Show()
{
    mp_console_root->show();
    mp_input_edit->activate();
}

void cGame_Console::Hide()
{
    mp_console_root->hide();
}

void cGame_Console::Toggle()
{
    if (mp_console_root->isVisible())
        Hide();
    else
        Show();
}

bool cGame_Console::IsVisible() const
{
    return mp_console_root->isVisible();
}

void cGame_Console::Update()
{

}

/**
 * Clear screen, output preamble and reset line number display.
 * Also clears command history.
 */
void cGame_Console::Reset()
{
    mp_output_edit->setText("");
    m_logfile << std::endl << "--- Console Reset ---" << std::endl;
    print_preamble();

    if (pActive_Level && pActive_Level->m_mruby /* exclude menu level */) {
        char buf[8];
        sprintf(buf, "%02d", pActive_Level->m_mruby->Get_Console_Context()->lineno);
        mp_lino_text->setText(std::string(buf) + ">>");
    }
    else {
        mp_lino_text->setText("1>>");
    }

    m_history.clear();
    m_history_idx = 0;
}

/**
 * Append the given text to the output widget. The argument has to
 * be valid UTF-8.
 */
void cGame_Console::Append_Text(std::string text)
{
    CEGUI::String ceguitext(reinterpret_cast<const CEGUI::utf8*>(text.c_str()));
    CEGUI::String curtext(mp_output_edit->getText());
    curtext += ceguitext;

    mp_output_edit->setText(curtext);
    mp_output_edit->setCaretIndex(curtext.length());
    mp_output_edit->ensureCaretIsVisible();

    m_logfile << text;
}

void cGame_Console::print_preamble()
{
    char text[10000];
    // TRANS: The text is copied from the end of the GPLv3. If there is an
    // TRANS: accepted translation of the GPLv3 available in your language,
    // TRANS: then you should use its wording rather than trying to translate
    // TRANS: it yourself.
    sprintf(text, _("TSC Scripting Console\nCopyright © 2012-2020 The TSC Contributors\n\n"
    "This program comes with ABSOLUTELY NO WARRANTY; for details\n"
    "see the file COPYING. This is free software, and you are\n"
    "welcome to redistribute it under certain conditions; see the\n"
    "aforementioned file for details.\n"), TSC_COMPILE_YEAR);

    Append_Text(std::string(text));
    Append_Text(std::string("\n"));

#ifdef TSC_VERSION_POSTFIX
    // TRANS: %s is the version postfix, e.g. "dev" or "beta3".
    sprintf(text, _("You are running TSC version %d.%d.%d-%s.\n"),
            TSC_VERSION_MAJOR,
            TSC_VERSION_MINOR,
            TSC_VERSION_PATCH,
            TSC_VERSION_POSTFIX);
#else
    sprintf(text, _("You are running TSC version %d.%d.%d.\n"),
            TSC_VERSION_MAJOR,
            TSC_VERSION_MINOR,
            TSC_VERSION_PATCH);
#endif

    Append_Text(std::string(text));
}

bool cGame_Console::on_input_accepted(const CEGUI::EventArgs& evt)
{
    char buf[8];
    CEGUI::String cegui_code(mp_input_edit->getText());
    std::string code = std::string(cegui_code.c_str()) + "\n";
    mp_input_edit->setText("");

    // Remember in history
    m_history.push_back(cegui_code);
    m_history_idx = m_history.size();
    m_last_edit.clear();

    if (!pActive_Level || !pActive_Level->m_mruby) { // This should never happen (2nd case may be menu level)
        Append_Text(std::string("ERROR: No active level!"));
        return true;
    }

    const mrbc_context* p_console_ctx = pActive_Level->m_mruby->Get_Console_Context();

    // Echo user input back
    sprintf(buf, "%02d", p_console_ctx->lineno);
    Append_Text(std::string(buf) + ">> " + code);

    // TODO: Given that the console should be the regular output in the future,
    // the following code should be merged into cMRuby_Interpreter::Run_Code().
    mrb_state* p_mrb_state = pActive_Level->m_mruby->Get_MRuby_State();
    mrb_value result = pActive_Level->m_mruby->Run_Code_In_Console_Context(code);

    if (p_mrb_state->exc) {
        Display_Exception(p_mrb_state);

        // Clear exception pointer so execution can continue
        p_mrb_state->exc = NULL;
    }
    else {
        mrb_value rstr = mrb_inspect(p_mrb_state, result);
        if (mrb_string_p(rstr)) {
            std::string str("=> ");
            str += std::string(RSTRING_PTR(rstr), RSTRING_LEN(rstr));
            str += "\n";
            Append_Text(str);
        }
        else {
            Append_Text(std::string(_("(#inspect did not return a string)\n")));
        }
    }

    sprintf(buf, "%02d", p_console_ctx->lineno);
    mp_lino_text->setText(std::string(buf) + ">>");

    return true;
}

bool cGame_Console::on_key_up(const CEGUI::EventArgs& evt)
{
    const CEGUI::KeyEventArgs& kevt = static_cast<const CEGUI::KeyEventArgs&>(evt);

    if (kevt.scancode == CEGUI::Key::Scan::ArrowUp) {
        History_Back();
        return true;
    }
    else if (kevt.scancode == CEGUI::Key::Scan::ArrowDown) {
        History_Forward();
        return true;
    }
    else {
        // For the topmost command, remember the exact edits.
        if (m_history_idx == m_history.size())
            m_last_edit = mp_input_edit->getText();

        return false;
    }
}

/**
 * Print class, message, and backtrace of the exception that terminated the
 * execution on the given stack to the game console.
 *
 * This method expects that `p_mrb_state` is in an exceptional state, i.e.
 * an mruby exception terminated its execution.
 *
 * This method does not clear the `exc` member of `p_mrb_state`; if this is
 * desired, you need to do it manually.
 */
void cGame_Console::Display_Exception(mrb_state* p_mrb_state)
{
    mrb_value exception   = mrb_obj_value(p_mrb_state->exc);
    mrb_value bt          = mrb_exc_backtrace(p_mrb_state, exception);
    mrb_value rdesc       = mrb_funcall(p_mrb_state, exception, "message", 0);
    const char* classname = mrb_obj_classname(p_mrb_state, exception);

    std::string message(classname);
    message += ": ";
    message += std::string(RSTRING_PTR(rdesc), RSTRING_LEN(rdesc));
    Append_Text(message);

    for(int i=0; i < RARRAY_LEN(bt); i++) {
        std::string btline("    from ");
        mrb_value rstep = mrb_ary_ref(p_mrb_state, bt, i);

        btline += std::string(RSTRING_PTR(rstep), RSTRING_LEN(rstep));
        btline += "\n";
        Append_Text(btline);
    }
}

/**
 * Show the command that was before the current command in the history,
 * if any. Otherwise do nothing.
 */
void cGame_Console::History_Back()
{
    if (m_history_idx > 0) {
        mp_input_edit->setText(m_history[--m_history_idx]);
        mp_input_edit->setCaretIndex(m_history[m_history_idx].length());
    }
}

/**
 * Show the command that was after the current command in the history,
 * if any. Otherwise clear the commandline.
 */
void cGame_Console::History_Forward()
{
    if (m_history_idx < m_history.size()) {
        if (++m_history_idx >= m_history.size()) {
            // Toplevel reached again; show the original input as stored
            // in `m_last_edit'.
            mp_input_edit->setText(m_last_edit);
            mp_input_edit->setCaretIndex(m_last_edit.length());
        }
        else {
            mp_input_edit->setText(m_history[m_history_idx]);
            mp_input_edit->setCaretIndex(m_history[m_history_idx].length());
        }
    }
}
