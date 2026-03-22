#pragma once

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include "Channel.hpp"
#include "Client.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sstream>
#include <cctype>
#include <signal.h>
#include <errno.h>
#include <stdexcept>
#include <exception>
#include <iomanip>
#include "Channel.hpp"
#define SERVER_NAME     "ft_irc"

class Server
{
public:
    Server(int port, const std::string &password);
    ~Server();

    void   start();  
    void   run();    
    void cleaning();
    void setupSignalHandler();  
    volatile sig_atomic_t flag;  
private:
    int                         _port;
    std::string                 _password;
    int                         _serverFd;
    std::vector<struct pollfd>  _pollfds;    
    std::map<int, Client *>         _clients;   
    std::map<std::string, Channel *> _channels;    

    void    _acceptClient();
    void    _disconnectClient(int fd);
    void    _readClient(int fd);
    void    _processMessages(Client *client);
    void    _handleCommand(Client *client, const std::string &line);
    std::vector<std::string> _splitByComma(const std::string &str);
    void    _cmdPass(Client *client, const std::vector<std::string> &args);
    void    _cmdNick(Client *client, const std::vector<std::string> &args);
    void    _cmdUser(Client *client, const std::vector<std::string> &args);
    void    _cmdJoin(Client *client, const std::vector<std::string> &args);
    void    _cmdPart(Client *client, const std::vector<std::string> &args);
    void    _cmdPrivmsg(Client *client, const std::vector<std::string> &args);
    void    _cmdKick(Client *client, const std::vector<std::string> &args);
    void    _cmdInvite(Client *client, const std::vector<std::string> &args);
    void    _cmdTopic(Client *client, const std::vector<std::string> &args);
    void    _cmdMode(Client *client, const std::vector<std::string> &args);
    void    _cmdQuit(Client *client, const std::vector<std::string> &args);
    void    _cmdPing(Client *client, const std::vector<std::string> &args);
    Client  *_getClientByNick(const std::string &nickname);
    bool _isValidNickname(const std::string &nick);
    bool _isValidChannelName(const std::string &name);
    void _checkRegistration(Client *client);
    std::vector<std::string>    _splitArgs(const std::string &line) const;
    void                        _sendReply(Client *client, int code,
                                           const std::string &msg) const;
    Server(const Server&);
    Server& operator=(const Server&);
};
