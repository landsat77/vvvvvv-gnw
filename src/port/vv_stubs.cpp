/*
 * The translator tools of VVVVVV (LocalizationMaint.cpp: they write the
 * language files) are not on the Game & Watch: their menu entries are
 * hidden (loc::show_translator_menu is false), these stand for them.
 */
#include <string>
#include "LocalizationMaint.h"

namespace loc
{

bool sync_lang_files(void) { return false; }
bool save_roomname_to_file(const std::string&, bool, int, int, const char*, const char*) { return false; }
bool save_roomname_explanation_to_files(bool, int, int, const char*) { return false; }
void local_limits_check(void) {}
void global_limits_check(void) {}
void populate_testable_script_ids(void) {}
bool populate_cutscene_test(const char*) { return false; }

} /* namespace loc */
