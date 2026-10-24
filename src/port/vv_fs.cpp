/*
 * FileSystemUtils.cpp of the port (replaces the PhysFS one).
 *
 * Two places:
 *  - the game's assets ("graphics/...", "sounds/...", "music/...") are in
 *    the pack (vv_plat_find_file: memory-mapped flash, read only);
 *  - the user's files ("saves/settings.vvv", "saves/unlock.vvv",
 *    "saves/tsave.vvv", "saves/qsave.vvv"...) are in the save folder of
 *    the platform (SD card): "saves/" is dropped from their names.
 * No player levels, no language folders, no level assets.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <tinyxml2.h>

#include "FileSystemUtils.h"
#include "BinaryBlob.h"
#include "Alloc.h"
#include "Vlogging.h"

#include "vv_platform.h"

static bool is_init;

int FILESYSTEM_init(char*, char*, char*, char*, char*)
{
    is_init = true;
    return 1;
}

bool FILESYSTEM_isInit(void) { return is_init; }
void FILESYSTEM_deinit(void) { is_init = false; }

static char save_dir_slash[200];
char* FILESYSTEM_getUserSaveDirectory(void)
{
    snprintf(save_dir_slash, sizeof(save_dir_slash), "%s/", vv_plat_save_dir());
    return save_dir_slash;
}
char* FILESYSTEM_getUserLevelDirectory(void) { return FILESYSTEM_getUserSaveDirectory(); }
char* FILESYSTEM_getUserMainLangDirectory(void) { return FILESYSTEM_getUserSaveDirectory(); }
bool FILESYSTEM_isMainLangDirFromRepo(void) { return false; }
bool FILESYSTEM_doesLangDirExist(void) { return false; }
bool FILESYSTEM_doesFontsDirExist(void) { return true; }
bool FILESYSTEM_setLangWriteDir(void) { return false; }
bool FILESYSTEM_restoreWriteDir(void) { return true; }

/* "saves/x" -> "<save dir>/x"; false for the other names */
static bool user_path(const char* name, char* out, size_t size)
{
    if (strncmp(name, "saves/", 6) != 0)
    {
        return false;
    }
    snprintf(out, size, "%s/%s", vv_plat_save_dir(), name + 6);
    return true;
}

static FILE* open_user(const char* name, const char* mode)
{
    char path[256];
    if (!user_path(name, path, sizeof(path)))
    {
        return NULL;
    }
    return fopen(path, mode);
}

bool FILESYSTEM_isFile(const char* filename)
{
    if (vv_plat_find_file(filename, NULL, NULL))
    {
        return true;
    }
    FILE* f = open_user(filename, "rb");
    if (f != NULL)
    {
        fclose(f);
        return true;
    }
    return false;
}

bool FILESYSTEM_isMounted(const char* filename) { return vv_plat_find_file(filename, NULL, NULL); }
void FILESYSTEM_loadZip(const char*) {}
bool FILESYSTEM_mountAssets(const char*) { return false; }
void FILESYSTEM_unmountAssets(void) {}
bool FILESYSTEM_isAssetMounted(const char*) { return false; }
bool FILESYSTEM_areAssetsInSameRealDir(const char*, const char*) { return true; }

bool FILESYSTEM_saveFile(const char* name, const unsigned char* data, size_t len)
{
    FILE* f = open_user(name, "wb");
    if (f == NULL)
    {
        vlog_error("Could not write %s", name);
        return false;
    }
    bool ok = fwrite(data, 1, len, f) == len;
    ok = (fclose(f) == 0) && ok;
    return ok;
}

/* A copy in the heap, with a NUL after the data (like upstream) */
static unsigned char* copy_out(const unsigned char* data, size_t size)
{
    unsigned char* mem = (unsigned char*) SDL_malloc(size + 1);
    if (mem == NULL)
    {
        return NULL;
    }
    memcpy(mem, data, size);
    mem[size] = '\0';
    return mem;
}

void FILESYSTEM_loadFileToMemory(const char* name, unsigned char** mem, size_t* len)
{
    *mem = NULL;
    if (len != NULL)
    {
        *len = 0;
    }
    const uint8_t* data;
    uint32_t size;
    if (vv_plat_find_file(name, &data, &size))
    {
        *mem = copy_out(data, size);
        if (*mem != NULL && len != NULL)
        {
            *len = size;
        }
        return;
    }
    FILE* f = open_user(name, "rb");
    if (f == NULL)
    {
        return;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0 || n > 1024 * 1024)
    {
        fclose(f);
        return;
    }
    unsigned char* buf = (unsigned char*) SDL_malloc((size_t) n + 1);
    if (buf != NULL && fread(buf, 1, (size_t) n, f) == (size_t) n)
    {
        buf[n] = '\0';
        *mem = buf;
        if (len != NULL)
        {
            *len = (size_t) n;
        }
    }
    else
    {
        SDL_free(buf);
    }
    fclose(f);
}

void FILESYSTEM_loadAssetToMemory(const char* name, unsigned char** mem, size_t* len)
{
    FILESYSTEM_loadFileToMemory(name, mem, len);
}

bool FILESYSTEM_loadBinaryBlob(binaryBlob*, const char*)
{
    /* the music is in the pack as separate songs (vv_music_game.cpp) */
    return false;
}

bool FILESYSTEM_saveTiXml2Document(const char* name, tinyxml2::XMLDocument& doc, bool)
{
    if (!is_init)
    {
        return false;
    }
    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);
    return FILESYSTEM_saveFile(name, (const unsigned char*) printer.CStr(), printer.CStrSize() - 1);
}

/* vv_state.cpp: this text stands for the file `name` (NULL: no more) */
static const char* override_name;
static const char* override_data;
static size_t override_size;

void vv_fs_override(const char* name, const char* data, size_t size)
{
    override_name = name;
    override_data = data;
    override_size = size;
}

bool FILESYSTEM_loadTiXml2Document(const char* name, tinyxml2::XMLDocument& doc)
{
    if (override_name != NULL && strcmp(name, override_name) == 0)
    {
        doc.Parse(override_data, override_size);
        return true;
    }
    /* the pack's files are parsed in place (tinyxml2 copies the text) */
    const uint8_t* data;
    uint32_t size;
    if (vv_plat_find_file(name, &data, &size))
    {
        doc.Parse((const char*) data, size);
        return true;
    }
    unsigned char* mem;
    size_t len;
    FILESYSTEM_loadFileToMemory(name, &mem, &len);
    if (mem == NULL)
    {
        return false;
    }
    doc.Parse((const char*) mem, len);
    VVV_free(mem);
    return true;
}

bool FILESYSTEM_loadAssetTiXml2Document(const char* name, tinyxml2::XMLDocument& doc)
{
    return FILESYSTEM_loadTiXml2Document(name, doc);
}

void FILESYSTEM_enumerateLevelDirFileNames(void (*)(const char*)) {}

/* the pack's files in `folder` (names without the folder) */
static const char* enumerate_pack(const char* folder, EnumHandle* handle)
{
    size_t flen = strlen(folder);
    uintptr_t i = (uintptr_t) handle->_item;
    for (;;)
    {
        const char* name = vv_plat_pack_name((uint32_t) i++);
        if (name == NULL)
        {
            handle->_item = (char**) i;
            return NULL;
        }
        if (strncmp(name, folder, flen) == 0 && name[flen] == '/' && strchr(name + flen + 1, '/') == NULL)
        {
            handle->_item = (char**) i;
            return name + flen + 1;
        }
    }
}

const char* FILESYSTEM_enumerate(const char* folder, EnumHandle* handle)
{
    return enumerate_pack(folder, handle);
}

const char* FILESYSTEM_enumerateAssets(const char*, EnumHandle*)
{
    return NULL; /* no level assets */
}

const char* FILESYSTEM_enumerateLanguageCodes(EnumHandle*)
{
    return NULL;
}

void FILESYSTEM_freeEnumerate(EnumHandle* handle)
{
    handle->_item = NULL;
    handle->physfs_list = NULL;
}

static char level_dir_error[160];
bool FILESYSTEM_levelDirHasError(void) { return level_dir_error[0] != '\0'; }
void FILESYSTEM_clearLevelDirError(void) { level_dir_error[0] = '\0'; }
const char* FILESYSTEM_getLevelDirError(void) { return level_dir_error; }
void FILESYSTEM_setLevelDirError(const char* text, const char*, ...)
{
    snprintf(level_dir_error, sizeof(level_dir_error), "%s", text);
}

bool FILESYSTEM_openDirectoryEnabled(void) { return false; }
bool FILESYSTEM_openDirectory(const char*) { return false; }

bool FILESYSTEM_delete(const char* name)
{
    char path[256];
    if (!user_path(name, path, sizeof(path)))
    {
        return false;
    }
    return remove(path) == 0;
}

void FILESYSTEM_deleteLevelSaves(void) {}
