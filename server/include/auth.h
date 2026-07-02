#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>

struct User {
    std::string email;
    std::string username;
    std::string salt;
    std::string hash;
};

struct Session {
    std::string username;
    time_t expires_at;
};

bool loadUsers(const std::string& path, std::vector<User>& users);
bool saveUsers(const std::string& path, const std::vector<User>& users);
std::string registerUser(std::vector<User>& users, const std::string& email,
                         const std::string& username, const std::string& password,
                         std::string& err);
std::string loginUser(const std::vector<User>& users, const std::string& login,
                      const std::string& password,
                      std::unordered_map<std::string, Session>& sessions,
                      std::string& out_username, std::string& err);
bool validateToken(const std::unordered_map<std::string, Session>& sessions,
                   const std::string& token, std::string& username);
void removeSession(std::unordered_map<std::string, Session>& sessions,
                   const std::string& token);
void cleanupExpiredSessions(std::unordered_map<std::string, Session>& sessions);
bool isValidEmail(const std::string& email);
bool isValidUsername(const std::string& username);
std::string getUserByToken(const std::unordered_map<std::string, Session>& sessions,
                           const std::string& token);
