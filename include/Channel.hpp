#pragma once
#include <string>
#include <vector>
#include <map>
#include "Client.hpp"

struct s_Modes
{
    bool    inviteOnly;      
    bool    topicRestricted; 
    bool    keyProtected;    
    bool    limited;         

    s_Modes() :
        inviteOnly(false),
        topicRestricted(false),
        keyProtected(false),
        limited(false)
    {}
};

class Channel
{
private:
    std::string              _name;
    std::string              _topic;
    std::string              _key;
    size_t                   _userLimit;
    s_Modes                  _modes;
    std::vector<Client *>    _clients;
    std::vector<Client *>    _operators;
    std::vector<std::string> _inviteList;
    Channel(const Channel &);
    Channel &operator=(const Channel &);
public:
    Channel(const std::string &name);
    ~Channel();
    const std::string&              getName()       const;
    const std::string&              getTopic()      const;
    const std::string&              getKey()        const;
    size_t                          getUserLimit()  const;
    const s_Modes&                  getModes()      const;
    const std::vector<Client *>&    getClients()    const;
    const std::vector<Client *>&    getOperators()  const;
    void    setTopic(const std::string &topic);
    void    setKey(const std::string &key);
    void    setUserLimit(size_t limit);
    void    setMode(char modeChar);
    void    unsetMode(char modeChar);
    bool    hasMode(char modeChar)  const;
    std::string modesActive();
    void    addClient(Client *client);
    void    removeClient(Client *client);
    bool    hasClient(Client *client)              const;
    bool    hasClient(const std::string &nickname) const;
    size_t  clientCount()                          const;
    bool    isFull()                               const;
    void    addInvite(const std::string &nickname);
    void    removeInvite(const std::string &nickname);
    bool    isInvited(const std::string &nickname) const;
    void    addOperator(Client *client);
    void    removeOperator(Client *client);
    bool    isOperator(Client *client)  const;
    void    broadcast(const std::string &message, Client *exclude = NULL) const;
    std::string getMemberList() const;
};
