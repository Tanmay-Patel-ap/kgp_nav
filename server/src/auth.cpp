#include "auth.h"
#include "sha256.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cstdlib>

using json = nlohmann::json;

// ── FILE I/O ──────────────────────────────────────────────────────────
bool loadUsers(const std::string& path, std::vector<User>& users) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cout << "[auth] No users file at " << path << " — starting fresh\n";
        return true;
    }
    try {
        json j; f >> j;
        for (const auto& u : j) {
            users.push_back({
                u.value("email", ""),
                u.value("username", ""),
                u.value("salt", ""),
                u.value("hash", "")
            });
        }
        std::cout << "[auth] Loaded " << users.size() << " user(s)\n";
        return true;
    } catch (...) {
        std::cerr << "[auth] Failed to parse users file\n";
        return false;
    }
}

bool saveUsers(const std::string& path, const std::vector<User>& users) {
    json j = json::array();
    for (const auto& u : users) {
        j.push_back({{"email", u.email}, {"username", u.username},
                     {"salt", u.salt}, {"hash", u.hash}});
    }
    std::ofstream f(path);
    if (!f.is_open()) { std::cerr << "[auth] Cannot write " << path << "\n"; return false; }
    f << j.dump(2);
    return true;
}

// ── VALIDATION ────────────────────────────────────────────────────────
bool isValidEmail(const std::string& email) {
    size_t at = email.find('@');
    if (at == std::string::npos || at == 0 || at == email.size() - 1) return false;
    size_t dot = email.find('.', at + 1);
    if (dot == std::string::npos || dot == at + 1 || dot == email.size() - 1) return false;
    if (email.find(' ') != std::string::npos) return false;
    return true;
}

bool isValidUsername(const std::string& username) {
    if (username.length() < 3 || username.length() > 20) return false;
    if (!((username[0] >= 'a' && username[0] <= 'z') || (username[0] >= 'A' && username[0] <= 'Z'))) return false;
    for (size_t i = 1; i < username.size(); i++) {
        char c = username[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '.')) return false;
    }
    return true;
}

// ── TOKEN / SALT GENERATION ──────────────────────────────────────────
static std::string randomString(int len) {
    static const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string out;
    out.reserve(len);
    for (int i = 0; i < len; i++)
        out += chars[rand() % (sizeof(chars) - 1)];
    return out;
}

// ── REGISTER ──────────────────────────────────────────────────────────
std::string registerUser(std::vector<User>& users, const std::string& email,
                         const std::string& username, const std::string& password,
                         std::string& err) {
    if (!isValidEmail(email)) { err = "Invalid email format"; return ""; }
    if (!isValidUsername(username)) { err = "Invalid username (3-20 chars, start with letter)"; return ""; }
    if (password.length() < 8) { err = "Password too short (min 8 chars)"; return ""; }

    for (const auto& u : users) {
        if (u.email == email) { err = "Email already registered"; return ""; }
        if (u.username == username) { err = "Username already taken"; return ""; }
    }

    std::string salt = randomString(16);
    std::string hash = sha256_hex(salt + password);
    users.push_back({email, username, salt, hash});
    saveUsers("server/data/users.json", users);

    return randomString(32); // initial token
}

// ── LOGIN ─────────────────────────────────────────────────────────────
std::string loginUser(const std::vector<User>& users, const std::string& login,
                      const std::string& password,
                      std::unordered_map<std::string, Session>& sessions,
                      std::string& out_username, std::string& err) {
    const User* found = nullptr;
    for (const auto& u : users) {
        if (u.email == login || u.username == login) { found = &u; break; }
    }
    if (!found) { err = "User not found"; return ""; }

    std::string hash = sha256_hex(found->salt + password);
    if (hash != found->hash) { err = "Wrong password"; return ""; }

    out_username = found->username;

    std::string token = randomString(32);
    while (sessions.count(token)) { token = randomString(32); }

    Session sess;
    sess.username = found->username;
    sess.expires_at = std::time(nullptr) + 7 * 24 * 3600;
    sessions[token] = sess;
    return token;
}

// ── TOKEN VALIDATION ──────────────────────────────────────────────────
bool validateToken(const std::unordered_map<std::string, Session>& sessions,
                   const std::string& token, std::string& username) {
    auto it = sessions.find(token);
    if (it == sessions.end()) return false;
    if (std::time(nullptr) > it->second.expires_at) {
        const_cast<std::unordered_map<std::string, Session>&>(sessions).erase(token);
        return false;
    }
    username = it->second.username;
    return true;
}

void removeSession(std::unordered_map<std::string, Session>& sessions,
                   const std::string& token) {
    sessions.erase(token);
}

void cleanupExpiredSessions(std::unordered_map<std::string, Session>& sessions) {
    time_t now = std::time(nullptr);
    for (auto it = sessions.begin(); it != sessions.end(); ) {
        if (now > it->second.expires_at) it = sessions.erase(it);
        else ++it;
    }
}

std::string getUserByToken(const std::unordered_map<std::string, Session>& sessions,
                           const std::string& token) {
    auto it = sessions.find(token);
    if (it == sessions.end() || std::time(nullptr) > it->second.expires_at) return "";
    return it->second.username;
}
