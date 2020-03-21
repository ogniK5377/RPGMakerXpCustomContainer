#include "AriMath.h"
#include "EnginePatches.h"
#include "RubyCommon.h"

namespace Patches {

// TODO(David): Custom Container loader
/*
using PrepareRGSSADType = bool (*)(LPCSTR lpFileName);
using ReadRGSSADType = bool (*)(char* filename, char** file_buffer, int* size);
using MallocType = void* (*)(unsigned int size);

PrepareRGSSADType ORIGINAL_PREPARE_RGSSAD = nullptr;
ReadRGSSADType ORIGINAL_READ_RGSSAD = nullptr;
MallocType RGSSAD_MALLOC = nullptr;
*/

void SetupDetours(const char* library_path) {
    Ruby::Common::Get()->AddNewModule(&RubyModule::RegisterAriMath);
    Ruby::Common::Get()->Initialize(library_path);
}

} // namespace Patches
