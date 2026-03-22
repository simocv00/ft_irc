#include "Channel.hpp"

const std::string &Channel::getName() const
{
    return _name;
}

const std::string &Channel::getTopic() const
{
    return _topic;
}

const std::string &Channel::getKey() const
{
    return _key;
}

size_t Channel::getUserLimit() const
{
    return _userLimit;
}

const s_Modes &Channel::getModes() const
{
    return _modes;
}

const std::vector<Client *> &Channel::getClients() const
{
    return _clients;
}

const std::vector<Client *> &Channel::getOperators() const
{
    return _operators;
}



void Channel::setTopic(const std::string &topic)
{
    _topic = topic;
}

void Channel::setKey(const std::string &key)
{
    _key = key;
}

void Channel::setUserLimit(size_t limit)
{
    _userLimit = limit;
}

bool Channel::hasClient(Client *client) const{
    for (size_t i = 0; i < _clients.size(); i++)
    {
        if(client == _clients[i])
            return true;
    }
    return false;
}

void Channel::addClient(Client *client){
    if(!hasClient(client))
        _clients.push_back(client);
    return;
}

void Channel::removeClient(Client *client)
{
    for (size_t i = 0; i < _clients.size(); i++)
    {
        if(client == _clients[i]){
            _clients.erase(_clients.begin() + i);
            return;
        }
    }
    
}

bool Channel::isOperator(Client *client) const{
      for (size_t i = 0; i < _operators.size(); i++)
    {
        if(client == _operators[i])
            return true;
    }
    return false;
}

void Channel::addOperator(Client *client){
     if(!isOperator(client))
        _operators.push_back(client);
    return;
}

void Channel::removeOperator(Client *client){
     for (size_t i = 0; i < _operators.size(); i++)
    {
        if(client == _operators[i]){
            _operators.erase(_operators.begin() + i);
            return;
        }
    }
}

void Channel::broadcast(const std::string &message, Client *exclude) const{
    for (size_t i = 0; i < _clients.size(); i++)
    {
        if(exclude != _clients[i]){
            _clients[i]->sendMessage(message);
        }
    }
    
}

std::string Channel::getMemberList() const{
    std::string list = "";
    for (size_t i = 0; i < _clients.size(); i++)
    {
        if(isOperator(_clients[i])){
            list += ("@" + _clients[i]->getNickname() + " ");
        }
        else{
            list += (_clients[i]->getNickname() + " ");
        }
    }
    return list;
}

bool Channel::isInvited(const std::string &nickname) const{
    for (size_t i = 0; i < _inviteList.size(); i++)
    {
        if(nickname == _inviteList[i]){
            return true;
        }
    }
    return false;
}
void Channel::addInvite(const std::string &nickname){
    if(!isInvited(nickname))
        _inviteList.push_back(nickname);
}
void Channel::removeInvite(const std::string &nickname){
    for (size_t i = 0; i < _inviteList.size(); i++)
    {
        if(nickname == _inviteList[i])
        {
            _inviteList.erase(_inviteList.begin() + i);
            return;
        }
    }  
}

bool Channel::hasMode(char modeChar) const{
    if(modeChar == 'i')
        return _modes.inviteOnly;
    else if(modeChar == 't')
        return _modes.topicRestricted;
    else if(modeChar == 'k')
        return _modes.keyProtected;
    else if(modeChar == 'l')
        return _modes.limited;
    return false;
}

void Channel::setMode(char modeChar)
{
    if(modeChar == 'i')
        _modes.inviteOnly = true;
    else if(modeChar == 't')
         _modes.topicRestricted = true;
    else if(modeChar == 'k')
        _modes.keyProtected = true;
    else if(modeChar == 'l')
        _modes.limited = true;

    return;
}

void Channel::unsetMode(char modeChar)
{
      if(modeChar == 'i')
        _modes.inviteOnly = false;
    else if(modeChar == 't')
         _modes.topicRestricted = false;
    else if(modeChar == 'k'){
        _modes.keyProtected = false;
        _key = "";
    }
    else if(modeChar == 'l'){
        _modes.limited = false;
        _userLimit = 0;
    }

    return;
}


Channel::Channel(const std::string &name) : _name(name), _topic(""), _key(""), _userLimit(0)
{
}

Channel::~Channel()
{
}

size_t Channel::clientCount() const
{
    return _clients.size();
}

bool Channel::isFull() const
{
    if (_userLimit == 0)
        return false;
    return _clients.size() >= _userLimit;
}

bool Channel::hasClient(const std::string &nickname) const
{
    for (size_t i = 0; i < _clients.size(); i++)
    {
        if (_clients[i]->getNickname() == nickname)
            return true;
    }
    return false;
}

std::string Channel::modesActive(){
    std::string modes = "+";
    if(hasMode('i'))
        modes += "i";
    if(hasMode('t'))
        modes += "t";
    if(hasMode('k'))
        modes += "k";
    if(hasMode('l'))
        modes += "l";
    return modes;   
}
