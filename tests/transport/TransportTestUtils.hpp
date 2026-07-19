#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <cstring>
#include <chrono>
#include <functional>
#include <future>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace mbootcore {
namespace test {

inline uint16_t findFreePort() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    SOCKET s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) return 9999;
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = 0;
    if (::bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        ::closesocket(s); WSACleanup(); return 9999;
    }
    socklen_t len = sizeof(addr);
    ::getsockname(s, reinterpret_cast<sockaddr*>(&addr), &len);
    uint16_t port = ntohs(addr.sin_port);
    ::closesocket(s);
    return port;
#else
    int s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) return 9999;
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = 0;
    if (::bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(s); return 9999;
    }
    socklen_t len = sizeof(addr);
    ::getsockname(s, reinterpret_cast<sockaddr*>(&addr), &len);
    uint16_t port = ntohs(addr.sin_port);
    ::close(s);
    return port;
#endif
}

class TcpEchoServer {
public:
    explicit TcpEchoServer(uint16_t port)
        : m_port(port) {}

    ~TcpEchoServer() { stop(); }

    void start() {
        if (m_running.load()) return;
        m_ready.store(false);
        m_startPromise = std::promise<void>();
        auto fut = m_startPromise.get_future();
        m_running.store(true);
        m_thread = std::thread([this]() { run(); });
        fut.wait();
    }

    void stop() {
        m_running.store(false);
        if (m_thread.joinable()) m_thread.join();
#ifdef _WIN32
        if (m_listenSock != INVALID_SOCKET) { ::closesocket(m_listenSock); m_listenSock = INVALID_SOCKET; }
        if (m_clientSock != INVALID_SOCKET) { ::closesocket(m_clientSock); m_clientSock = INVALID_SOCKET; }
#else
        if (m_listenSock >= 0) { ::close(m_listenSock); m_listenSock = -1; }
        if (m_clientSock >= 0) { ::close(m_clientSock); m_clientSock = -1; }
#endif
    }

    uint16_t port() const { return m_port; }
    bool isRunning() const { return m_running.load(); }

private:
    void run() {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
        m_listenSock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (m_listenSock == INVALID_SOCKET) { m_running.store(false); m_startPromise.set_value(); return; }
        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        addr.sin_port = htons(m_port);
        if (::bind(m_listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR ||
            ::listen(m_listenSock, 1) == SOCKET_ERROR) {
            ::closesocket(m_listenSock); m_listenSock = INVALID_SOCKET;
            m_running.store(false); m_startPromise.set_value(); return;
        }
        m_ready.store(true);
        m_startPromise.set_value();
        char buf[4096];
        while (m_running.load() && m_listenSock != INVALID_SOCKET) {
            fd_set acceptSet;
            FD_ZERO(&acceptSet);
            FD_SET(m_listenSock, &acceptSet);
            struct timeval atv = {0, 500000};
            int ar = ::select(0, &acceptSet, nullptr, nullptr, &atv);
            if (ar < 0) break;
            if (ar == 0) continue;

            sockaddr_in clientAddr;
            int clientLen = sizeof(clientAddr);
            SOCKET cs = ::accept(m_listenSock, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
            if (cs == INVALID_SOCKET) { continue; }
            m_clientSock = cs;
            bool peerClosed = false;
            while (m_running.load() && !peerClosed) {
                fd_set set;
                FD_ZERO(&set);
                FD_SET(m_clientSock, &set);
                struct timeval tv = {0, 100000};
                int ret = ::select(0, &set, nullptr, nullptr, &tv);
                if (ret <= 0) continue;
                int n = ::recv(m_clientSock, buf, sizeof(buf), 0);
                if (n <= 0) break;
                int sent = 0;
                while (sent < n) {
                    int s = ::send(m_clientSock, buf + sent, n - sent, 0);
                    if (s <= 0) { peerClosed = true; break; }
                    sent += s;
                }
            }
            ::closesocket(m_clientSock); m_clientSock = INVALID_SOCKET;
        }
        ::closesocket(m_listenSock); m_listenSock = INVALID_SOCKET;
#else
        m_listenSock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (m_listenSock < 0) { m_running.store(false); m_startPromise.set_value(); return; }
        int opt = 1;
        setsockopt(m_listenSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        addr.sin_port = htons(m_port);
        if (::bind(m_listenSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 ||
            ::listen(m_listenSock, 1) < 0) {
            ::close(m_listenSock); m_listenSock = -1;
            m_running.store(false); m_startPromise.set_value(); return;
        }
        m_ready.store(true);
        m_startPromise.set_value();
        char buf[4096];
        while (m_running.load() && m_listenSock >= 0) {
            fd_set acceptSet;
            FD_ZERO(&acceptSet);
            FD_SET(m_listenSock, &acceptSet);
            struct timeval atv = {0, 500000};
            int ar = ::select(m_listenSock + 1, &acceptSet, nullptr, nullptr, &atv);
            if (ar < 0) break;
            if (ar == 0) continue;

            sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int cs = ::accept(m_listenSock, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
            if (cs < 0) { continue; }
            m_clientSock = cs;
            bool peerClosed = false;
            while (m_running.load() && !peerClosed) {
                fd_set set;
                FD_ZERO(&set);
                FD_SET(m_clientSock, &set);
                struct timeval tv = {0, 100000};
                int ret = ::select(m_clientSock + 1, &set, nullptr, nullptr, &tv);
                if (ret <= 0) continue;
                ssize_t n = ::read(m_clientSock, buf, sizeof(buf));
                if (n <= 0) break;
                ssize_t sent = 0;
                while (sent < n) {
                    ssize_t s = ::write(m_clientSock, buf + sent, static_cast<size_t>(n - sent));
                    if (s <= 0) { peerClosed = true; break; }
                    sent += s;
                }
            }
            ::close(m_clientSock); m_clientSock = -1;
        }
        ::close(m_listenSock); m_listenSock = -1;
#endif
    }

    uint16_t m_port;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_ready{false};
    std::promise<void> m_startPromise;
#ifdef _WIN32
    SOCKET m_listenSock{INVALID_SOCKET};
    SOCKET m_clientSock{INVALID_SOCKET};
#else
    int m_listenSock{-1};
    int m_clientSock{-1};
#endif
};

class UdpEchoServer {
public:
    explicit UdpEchoServer(uint16_t port)
        : m_port(port) {}

    ~UdpEchoServer() { stop(); }

    void start() {
        if (m_running.load()) return;
        m_ready.store(false);
        m_startPromise = std::promise<void>();
        auto fut = m_startPromise.get_future();
        m_running.store(true);
        m_thread = std::thread([this]() { run(); });
        fut.wait();
    }

    void stop() {
        m_running.store(false);
        if (m_thread.joinable()) m_thread.join();
#ifdef _WIN32
        if (m_sock != INVALID_SOCKET) ::closesocket(m_sock);
#else
        if (m_sock >= 0) ::close(m_sock);
#endif
    }

    uint16_t port() const { return m_port; }
    bool isRunning() const { return m_running.load(); }

private:
    void run() {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
        m_sock = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (m_sock == INVALID_SOCKET) { m_running.store(false); m_startPromise.set_value(); return; }
        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        addr.sin_port = htons(m_port);
        if (::bind(m_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            ::closesocket(m_sock); m_sock = INVALID_SOCKET;
            m_running.store(false); m_startPromise.set_value(); return;
        }
        m_ready.store(true);
        m_startPromise.set_value();
        char buf[4096];
        sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        while (m_running.load()) {
            fd_set set;
            FD_ZERO(&set);
            FD_SET(m_sock, &set);
            struct timeval tv = {0, 100000};
            int ret = ::select(0, &set, nullptr, nullptr, &tv);
            if (ret <= 0) continue;
            clientLen = sizeof(clientAddr);
            int n = ::recvfrom(m_sock, buf, sizeof(buf), 0,
                               reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
            if (n <= 0) continue;
            ::sendto(m_sock, buf, n, 0,
                     reinterpret_cast<sockaddr*>(&clientAddr), clientLen);
        }
#else
        m_sock = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (m_sock < 0) { m_running.store(false); m_startPromise.set_value(); return; }
        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        addr.sin_port = htons(m_port);
        if (::bind(m_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            ::close(m_sock); m_sock = -1;
            m_running.store(false); m_startPromise.set_value(); return;
        }
        m_ready.store(true);
        m_startPromise.set_value();
        char buf[4096];
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        while (m_running.load()) {
            fd_set set;
            FD_ZERO(&set);
            FD_SET(m_sock, &set);
            struct timeval tv = {0, 100000};
            int ret = ::select(m_sock + 1, &set, nullptr, nullptr, &tv);
            if (ret <= 0) continue;
            clientLen = sizeof(clientAddr);
            ssize_t n = ::recvfrom(m_sock, buf, sizeof(buf), 0,
                                   reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
            if (n <= 0) continue;
            ::sendto(m_sock, buf, static_cast<size_t>(n), 0,
                     reinterpret_cast<sockaddr*>(&clientAddr), clientLen);
        }
#endif
    }

    uint16_t m_port;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_ready{false};
    std::promise<void> m_startPromise;
#ifdef _WIN32
    SOCKET m_sock{INVALID_SOCKET};
#else
    int m_sock{-1};
#endif
};

} // namespace test
} // namespace mbootcore
