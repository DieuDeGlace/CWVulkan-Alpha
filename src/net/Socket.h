#pragma once
// Blocking TCP sockets for Alpha's protocol (alpha-net-transport.md §1): IPv4, every read reads
// exactly N bytes, a closed peer is a disconnect (§5.3). One Socket per connection.
#include <cstdint>
#include <string>
#include <vector>

namespace net {

// Starts Winsock once for the process.
bool startNetworking();

class Socket {
  public:
    Socket() = default;
    explicit Socket(uintptr_t handle) : handle(handle) {}
    Socket(Socket &&other) noexcept : handle(other.handle) { other.handle = INVALID; }
    Socket &operator=(Socket &&other) noexcept;
    Socket(const Socket &) = delete;
    Socket &operator=(const Socket &) = delete;
    ~Socket() { close(); }

    // Resolves `host` (a dotted address or a name, first IPv4 result) and connects.
    bool connect(const std::string &host, uint16_t port, std::string *error = nullptr);
    // Listens on all interfaces; `port` 0 picks a free one (see boundPort).
    bool listen(uint16_t port, std::string *error = nullptr, const std::string &bindAddress = "0.0.0.0");
    uint16_t boundPort() const;
    // Waits for the next connection; an invalid socket when listening stopped.
    Socket accept();

    bool valid() const { return handle != INVALID; }
    // Reads give up after `ms` milliseconds (0: wait forever).
    void setReadTimeout(uint32_t ms);
    void close();
    // Stops reads and writes in progress (from another thread) without freeing the handle.
    void shutdown();

    bool send(const void *data, size_t size);
    bool send(const std::vector<uint8_t> &bytes) { return send(bytes.data(), bytes.size()); }
    bool sendU32(uint32_t v) { return send(&v, 4); }
    bool readExact(void *out, size_t size);
    bool readU32(uint32_t &v) { return readExact(&v, 4); }

  private:
    static constexpr uintptr_t INVALID = ~(uintptr_t)0;
    uintptr_t handle = INVALID;
};

} // namespace net
