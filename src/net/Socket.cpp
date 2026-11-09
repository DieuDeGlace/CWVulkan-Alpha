#include "net/Socket.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mutex>

namespace net {

bool startNetworking() {
    static std::once_flag once;
    static bool started = false;
    std::call_once(once, [] {
        WSADATA data;
        started = WSAStartup(MAKEWORD(2, 2), &data) == 0;
    });
    return started;
}

Socket &Socket::operator=(Socket &&other) noexcept {
    if (this != &other) {
        close();
        handle = other.handle;
        other.handle = INVALID;
    }
    return *this;
}

void Socket::close() {
    if (handle != INVALID)
        closesocket((SOCKET)handle);
    handle = INVALID;
}

void Socket::setReadTimeout(uint32_t ms) {
    if (handle != INVALID)
        setsockopt((SOCKET)handle, SOL_SOCKET, SO_RCVTIMEO, (const char *)&ms, sizeof ms);
}

void Socket::shutdown() {
    if (handle != INVALID)
        ::shutdown((SOCKET)handle, SD_BOTH);
}

bool Socket::connect(const std::string &host, uint16_t port, std::string *error) {
    close();
    if (!startNetworking()) {
        if (error)
            *error = "networking could not start";
        return false;
    }
    addrinfo hints{}, *found = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &found) != 0 || !found) {
        if (error)
            *error = "could not resolve " + host;
        return false;
    }
    const SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    const bool ok = s != INVALID_SOCKET && ::connect(s, found->ai_addr, (int)found->ai_addrlen) == 0;
    freeaddrinfo(found);
    if (!ok) {
        if (s != INVALID_SOCKET)
            closesocket(s);
        if (error)
            *error = "could not connect to " + host;
        return false;
    }
    handle = (uintptr_t)s;
    return true;
}

bool Socket::listen(uint16_t port, std::string *error, const std::string &bindAddress) {
    close();
    if (!startNetworking()) {
        if (error)
            *error = "networking could not start";
        return false;
    }
    const SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        if (error)
            *error = "Could not create server socket.";
        return false;
    }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    if (InetPtonA(AF_INET, bindAddress.c_str(), &address.sin_addr) != 1) {
        closesocket(s);
        if (error)
            *error = "Bind address must be numeric IPv4.";
        return false;
    }
    address.sin_port = htons(port);
    if (bind(s, (sockaddr *)&address, sizeof address) != 0) {
        closesocket(s);
        if (error)
            *error = "Could not bind socket.";
        return false;
    }
    if (::listen(s, 10) != 0) {
        closesocket(s);
        if (error)
            *error = "Listen failed.";
        return false;
    }
    handle = (uintptr_t)s;
    return true;
}

uint16_t Socket::boundPort() const {
    sockaddr_in address{};
    int length = sizeof address;
    if (handle == INVALID || getsockname((SOCKET)handle, (sockaddr *)&address, &length) != 0)
        return 0;
    return ntohs(address.sin_port);
}

Socket Socket::accept() {
    if (handle == INVALID)
        return Socket();
    const SOCKET s = ::accept((SOCKET)handle, nullptr, nullptr);
    return s == INVALID_SOCKET ? Socket() : Socket((uintptr_t)s);
}

bool Socket::send(const void *data, size_t size) {
    const char *p = (const char *)data;
    while (size > 0) {
        const int sent = ::send((SOCKET)handle, p, (int)size, 0);
        if (sent <= 0)
            return false;
        p += sent;
        size -= (size_t)sent;
    }
    return true;
}

bool Socket::readExact(void *out, size_t size) {
    char *p = (char *)out;
    while (size > 0) {
        const int got = ::recv((SOCKET)handle, p, (int)size, 0);
        if (got <= 0) // 0: the peer closed; treated as a disconnect (§5.3)
            return false;
        p += got;
        size -= (size_t)got;
    }
    return true;
}

} // namespace net
