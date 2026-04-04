#pragma once
#include <string>
#include <vector>
#include <netinet/in.h>
#include <ctime>

class Channel;

class Client
{
private:
    int         _fd;
    std::string _nickname;
    std::string _username;
    std::string _hostname;
    std::string _realname;
    bool _password;
    bool        _registered;
    bool        _operator;
    std::string _readBuf;
    std::time_t _connectionTime;
    Client(const Client&);
    Client& operator=(const Client&);
public:
    Client(int fd, const std::string &hostname);
    ~Client();
    int                         getFd()         const;
    const std::string&          getNickname()   const;
    const std::string&          getUsername()   const;
    const std::string&          getHostname()   const;
    const std::string&          getRealname()   const;
    bool          getPassword()   const;
    bool                        isRegistered()  const;
    bool                        isOperator()    const;
    std::time_t                 getConnectionTime() const;
    const std::string&          getReadBuf()    const;
    void    setNickname(const std::string &nickname);
    void    setUsername(const std::string &username);
    void    setRealname(const std::string &realname);
    void    setPassword(bool password);
    void    setRegistered(bool state);
    void    setOperator(bool state);
    void    setReadBuf(const std::string &buf);
    void    appendToReadBuf(const std::string &data);
    void    clearReadBuf(); 
    void    sendMessage(const std::string &message) const;
    std::string getPrefix() const; 
};
