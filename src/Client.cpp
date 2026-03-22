#include "include/Client.hpp"

Client::Client(int fd, const std::string &hostname)
    : _fd(fd),
      _nickname("*"),
      _username(""),
      _hostname(hostname),
      _realname(""),
      _password(false),
      _registered(false),
      _operator(false),
      _readBuf("")
{}
Client::~Client() {}
int                     Client::getFd()       const { return _fd; }
const std::string&      Client::getNickname() const { return _nickname; }
const std::string&      Client::getUsername() const { return _username; }
const std::string&      Client::getHostname() const { return _hostname; }
const std::string&      Client::getRealname() const { return _realname; }
bool      Client::getPassword() const { return _password; }
bool                    Client::isRegistered()const { return _registered; }
bool                    Client::isOperator()  const { return _operator; }
const std::string&      Client::getReadBuf()  const { return _readBuf; }
void    Client::setNickname(const std::string &nickname) { _nickname = nickname; }
void    Client::setUsername(const std::string &username) { _username = username; }
void    Client::setRealname(const std::string &realname) { _realname = realname; }
void    Client::setPassword(bool password) { _password = password; }
void    Client::setRegistered(bool state)                { _registered = state; }
void    Client::setOperator(bool state)                  { _operator = state; }
void    Client::setReadBuf(const std::string &buf)       { _readBuf = buf; }
void    Client::appendToReadBuf(const std::string &data) { _readBuf += data; }
void    Client::clearReadBuf()                           { _readBuf.clear(); }
void    Client::sendMessage(const std::string &message) const
{
    std::string out = message + "\r\n";
    send(_fd, out.c_str(), out.size(), MSG_NOSIGNAL);
}

std::string Client::getPrefix() const
{
    return _nickname + "!" + _username + "@" + _hostname;
}
