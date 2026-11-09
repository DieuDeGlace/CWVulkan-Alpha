#pragma once
// Little-endian byte reading and writing for Alpha's saves and network records: fields follow
// each other with no alignment (alpha-saves.md §0, alpha-net-transport.md).
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

class ByteReader {
  public:
    ByteReader(const uint8_t *data, size_t size) : p(data), end(data + size) {}
    explicit ByteReader(const std::vector<uint8_t> &bytes) : ByteReader(bytes.data(), bytes.size()) {}

    // False once a read ran past the end; every read after that gives zeros.
    bool ok() const { return good; }
    size_t left() const { return (size_t)(end - p); }
    bool atEnd() const { return good && p == end; }

    template <class T> T get() {
        T value{};
        read(&value, sizeof value);
        return value;
    }
    uint8_t u8() { return get<uint8_t>(); }
    uint32_t u32() { return get<uint32_t>(); }
    int32_t i32() { return get<int32_t>(); }
    uint64_t u64() { return get<uint64_t>(); }
    int64_t i64() { return get<int64_t>(); }
    float f32() { return get<float>(); }
    // u32 length, then that many bytes.
    std::string string() {
        const uint32_t n = u32();
        if (n > left()) {
            good = false;
            return {};
        }
        std::string s((const char *)p, n);
        p += n;
        return s;
    }
    std::vector<uint8_t> bytes(size_t n) {
        std::vector<uint8_t> out(n);
        read(out.data(), n);
        return out;
    }
    void read(void *out, size_t n) {
        if (!good || n > left()) {
            good = false;
            std::memset(out, 0, n);
            return;
        }
        std::memcpy(out, p, n);
        p += n;
    }

  private:
    const uint8_t *p, *end;
    bool good = true;
};

class ByteWriter {
  public:
    template <class T> void put(T value) { write(&value, sizeof value); }
    void u8(uint8_t v) { put(v); }
    void u32(uint32_t v) { put(v); }
    void i32(int32_t v) { put(v); }
    void u64(uint64_t v) { put(v); }
    void i64(int64_t v) { put(v); }
    void f32(float v) { put(v); }
    void string(const std::string &s) {
        u32((uint32_t)s.size());
        write(s.data(), s.size());
    }
    void bytes(const std::vector<uint8_t> &b) { write(b.data(), b.size()); }
    void write(const void *data, size_t n) {
        const auto *b = (const uint8_t *)data;
        out.insert(out.end(), b, b + n);
    }
    std::vector<uint8_t> &data() { return out; }

  private:
    std::vector<uint8_t> out;
};
