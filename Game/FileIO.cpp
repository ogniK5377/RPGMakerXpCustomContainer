#include "FileIO.h"

namespace IO {
File::File(const char* filename, const char* mode) {
    if (fopen_s(&fp, filename, mode)) {
        return;
    }
    Seek(0, Origin::End);
    file_size = Tell();
    Seek(0, Origin::Set);
}

File::~File() {
    if (fp) {
        fclose(fp);
    }
}

void File::Seek(std::size_t offset, File::Origin origin) {
    if (!fp) {
        return;
    }
    fseek(fp, offset, static_cast<int>(origin));
}

std::size_t File::Tell() {
    if (!fp) {
        return 0;
    }
    return ftell(fp);
}

std::size_t File::Size() const {
    if (!fp) {
        return 0;
    }
    return file_size;
}

bool File::Open() const {
    return fp != nullptr;
}

} // namespace IO
