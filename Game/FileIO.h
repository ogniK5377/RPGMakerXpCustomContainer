#pragma once
#include <cstdio>
#include <cstdlib>
#include <string>

namespace IO {

class File {
public:
    enum class Origin {
        Set = SEEK_SET,
        Cur = SEEK_CUR,
        End = SEEK_END,
    };

    File(const char* filename, const char* mode);
    ~File();

    void Seek(std::size_t offset, File::Origin origin = File::Origin::Set);
    std::size_t Tell();
    std::size_t Size() const;
    bool Open() const;

    template <typename T>
    std::size_t Write(T data, std::size_t count = 1) {
        if (!fp) {
            return 0;
        }
        return fwrite(&data, sizeof(T), count, fp);
    }

    template <>
    std::size_t Write(std::string data, std::size_t count) {
        if (!fp) {
            return 0;
        }
        const std::size_t str_len = data.length();
        fwrite(&str_len, sizeof(std::size_t), count, fp);
        return fwrite(data.c_str(), 1, str_len, fp);
    }

    std::size_t WriteBuffer(const void* data, std::size_t size) {
        if (!fp) {
            return 0;
        }
        return fwrite(data, size, 1, fp);
    }

    void ReadBuffer(void* data, std::size_t size) {
        if (!fp) {
            return;
        }
        fread(data, size, 1, fp);
    }

    template <typename T>
    T Read() {
        T t{};
        if (!fp) {
            return t;
        }
        std::fread(&t, sizeof(T), 1, fp);
        return t;
    }

    template <>
    std::string Read() {
        if (!fp) {
            return {};
        }
        const std::size_t str_len = Read<std::size_t>();
        std::string output{};
        output.resize(str_len);
        fread(output.data(), 1, str_len, fp);
        return output;
    }

private:
    std::size_t file_size{};
    FILE* fp = nullptr;
};

} // namespace IO
