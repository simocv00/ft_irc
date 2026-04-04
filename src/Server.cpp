#include "Server.hpp"


static Server* g_server = NULL;
bool g_client_disconnected = false;
static void signalHandler(int sig)
{
    (void)sig;
    if (g_server)
        g_server->flag = 0;
}

void Server::start(void)
{
    this->_serverFd = socket(AF_INET,SOCK_STREAM,0);
    if(this->_serverFd < 0){
        throw std::runtime_error(strerror(errno));
    }
    int reuse = 1;
    if (setsockopt(this->_serverFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        close(this->_serverFd);
        throw std::runtime_error(strerror(errno));
    }
    sockaddr_in server;
    std::memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(_port);
    server.sin_addr.s_addr = INADDR_ANY;
    if(bind(this->_serverFd,(sockaddr*)&server,(socklen_t)sizeof(server)) < 0)
    {
        close(this->_serverFd);
        throw std::runtime_error(strerror(errno));
    }
    if(listen(this->_serverFd,ECONNREFUSED) < 0)
    {
        close(this->_serverFd);
        throw std::runtime_error(strerror(errno));
    }
}

void Server::run()
{
    pollfd p = { _serverFd, POLLIN, 0 };
    this->_pollfds.push_back(p);
    
    while (this->flag)
    {
        if(poll(this->_pollfds.data(),this->_pollfds.size(),-1) < 0){
            if (errno == EINTR)
                continue;
            std::cerr << "poll error !" << std::endl;
            break;
        }
        if(this->_pollfds[0].revents & POLLIN){
            _acceptClient();
        }
        for (size_t i = 1; i < this->_pollfds.size(); i++)
        {
            g_client_disconnected = false;
            if(this->_pollfds[i].revents & POLLIN){
                _readClient(this->_pollfds[i].fd);
            }
            else if (this->_pollfds[i].revents & (POLLHUP | POLLERR | POLLNVAL)){
                _disconnectClient(this->_pollfds[i].fd);
            }
            if (g_client_disconnected) {
                i--;
            }
        }
        
    }
    
}

void Server::_acceptClient(void)
{
    sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr); 
    int new_client_fd = accept(this->_serverFd,(sockaddr*)&client_addr,&addr_len);
    if(new_client_fd < 0){
        throw std::runtime_error("accept :" + std::string(std::strerror(errno)));
    }
    if (fcntl(new_client_fd, F_SETFL, O_NONBLOCK) < 0) {
        std::cerr << "Error: fcntl failed for client." << std::endl;
        close(new_client_fd);
        return;
    }
   
    Client *ptr = new Client(new_client_fd,inet_ntoa(client_addr.sin_addr));
    this->_clients[new_client_fd] = ptr;
    pollfd p = {new_client_fd, POLLIN, 0};
    this->_pollfds.push_back(p);
    std::cout << "New client connected! " <<
         " IP: " << inet_ntoa(client_addr.sin_addr) << std::endl;
}

void Server::_readClient(int fd)
{
    char buffer[1024];
    std::memset(buffer,0,sizeof(buffer));
    int bytes_read = recv(fd,buffer,sizeof(buffer),0);
    if(bytes_read <= 0){
            _disconnectClient(fd);
            return;
    }
    Client *client = this->_clients[fd];
    client->setReadBuf(client->getReadBuf() + buffer);
    
    if (client->getReadBuf().length() > 4096) {
        _disconnectClient(fd);
        return;
    }
    
    _processMessages(client);
}

void Server::_disconnectClient(int fd)
{
    g_client_disconnected = true;
    close(fd);
    for (std::vector<pollfd>::iterator it = _pollfds.begin(); it != _pollfds.end(); ++it)
    {
    if (it->fd == fd)
        {
            _pollfds.erase(it);
            break;
        }
    }
    std::map<int, Client*>::iterator it = this->_clients.find(fd);
    if (it != this->_clients.end())
    {
        Client* client = it->second;
        
        std::map<std::string, Channel*> channelsCopy = _channels;
        for (std::map<std::string, Channel*>::iterator cIt = channelsCopy.begin(); cIt != channelsCopy.end(); ++cIt) {
            Channel* c = cIt->second;
            if (c->hasClient(client)) {
                c->broadcast(":" + client->getPrefix() + " QUIT :Client disconnected", client);
                c->removeClient(client);
                c->removeOperator(client);
                if (c->clientCount() == 0) {
                    delete c;
                    _channels.erase(cIt->first);
                }
            }
        }
        
        std::cout << "Client disconnected !"
         " IP: " << client->getHostname() << std::endl;
        delete client;
        this->_clients.erase(it);
    }
}

void Server::_processMessages(Client *client)
{
    std::string buffer = client->getReadBuf();
    size_t pos;

    while ((pos = buffer.find("\r\n")) != std::string::npos)
    {
        std::string line = buffer.substr(0, pos);

        buffer.erase(0, pos + 2);

        client->setReadBuf(buffer);

        if (!line.empty()) {
            _handleCommand(client, line);
            if (g_client_disconnected) {
                break;
            }
        }
    }
}

std::vector<std::string> Server::_splitArgs(const std::string &line) const
{
    std::vector<std::string> line_splited;
    if (line.empty())
        return line_splited;

    size_t pos = line.find(" :"); 
    std::string message;
    std::string args = line;

    if (pos != std::string::npos)
    {
        args = line.substr(0, pos);
        message = line.substr(pos + 2);
    }
    else if (line[0] == ':')
    {
        args = "";
        message = line.substr(1);
    }

    std::stringstream s(args);
    std::string arg;
    while (s >> arg)
    {
        line_splited.push_back(arg);
    }

    if (pos != std::string::npos || line[0] == ':')
    {
        line_splited.push_back(message);
    }

    return line_splited;
}

void Server::_handleCommand(Client *client, const std::string &line)
{
    std::vector<std::string> args = _splitArgs(line);
    
    if (args.empty())
        return;

    for (size_t i = 0; i < args[0].size(); i++) {
        args[0][i] = std::toupper(args[0][i]);
    }

    std::string cmd = args[0];
    if (cmd == "PASS") {
        _cmdPass(client, args);
        return;
    }
    else if (cmd == "NICK") {
        _cmdNick(client, args);
        return;
    }
    else if (cmd == "USER") {
        _cmdUser(client, args);
        return;
    }
    else if (cmd == "QUIT") {
        _cmdQuit(client, args);
        return;
    }
    if (cmd == "CAP") {
        if (args.size() > 1 && args[1] == "LS") {
            std::string capMsg = ":ft_irc CAP * LS :\r\n";
            send(client->getFd(), capMsg.c_str(), capMsg.length(), MSG_NOSIGNAL);
        }
        return;
    }

    if (!client->isRegistered()) {
        _sendReply(client, 451, ":You have not registered");
        return;
    }

    if (cmd == "JOIN") {
        _cmdJoin(client, args);
    }
    else if (cmd == "PRIVMSG") {
        _cmdPrivmsg(client, args);
    }
    else if (cmd == "KICK") {
        _cmdKick(client, args);
    }
    else if (cmd == "PART") {
        _cmdPart(client, args);
    }
    else if (cmd == "INVITE") {
        _cmdInvite(client, args);
    }
    else if (cmd == "TOPIC") {
        _cmdTopic(client, args);
    }
    else if (cmd == "MODE") {
        _cmdMode(client, args);
    }
    else if (cmd == "PING") {
        _cmdPing(client, args);
    }
    else {
        _sendReply(client, 421, cmd + " :Unknown command");
    }
}
Server::Server(int port, const std::string &password){
    this->_port = port;
    this->_password = password;
    this->flag = 1;
}
Server::~Server(){
    cleaning();
}

void Server::_sendReply(Client *client, int code, const std::string &msg) const
{
    std::stringstream ss; 
    ss << ":" << "ft_irc" << " " << std::setfill('0') << std::setw(3) << code << " ";
    if (client->getNickname().empty())
        ss << "* ";
    else
        ss << client->getNickname() << " ";
    ss << msg << "\r\n";
    std::string final_msg = ss.str();
    send(client->getFd(), final_msg.c_str(), final_msg.length(), MSG_NOSIGNAL);
}
 void Server::_checkRegistration(Client *client)
 {
    if (client->isRegistered()) {
        return;
    }
    if (client->getPassword() && !client->getNickname().empty() && !client->getUsername().empty())
    {
        client->setRegistered(true);
        _sendReply(client, 1, ":Welcome to the ft_irc Network, " + client->getPrefix());
    }
}

void    Server::_cmdPass(Client *client, const std::vector<std::string> &args)
{
    if(client->isRegistered()){
        _sendReply(client,462,":You may not reregister");
        return;
    }
    if(args.size() < 2){
        _sendReply(client,461,":Not enough parameters");
        return;
    }
    if(args[1] == _password){
        client->setPassword(true);
    }
    else
    {
         _sendReply(client,464,"PASS:Password incorrect");
        return;
    }
}

void Server::_cmdNick(Client *client, const std::vector<std::string> &args)
{
    if (!client->getPassword()) {
        _sendReply(client, 451, ":Enter your Password first");
        return;
    }

    if (args.size() < 2) {
        _sendReply(client, 431, ":No nickname given");
        return;
    }
    if (!_isValidNickname(args[1])) {
        _sendReply(client, 432, args[1] + " :Erroneous nickname");
        return;
    }
    for (std::map<int, Client*>::iterator it = this->_clients.begin(); it != this->_clients.end(); ++it) {
        
        if (it->second != client && it->second->getNickname() == args[1]) {
            _sendReply(client, 433, args[1] + " :Nickname is already in use");
            return;
        }
    }

    client->setNickname(args[1]);
    _checkRegistration(client);
}

void    Server::_cmdUser(Client *client, const std::vector<std::string> &args)
{
    if (!client->getPassword()) {
        _sendReply(client, 451, ":Enter your Password first");
        return;
    }
    if(client->isRegistered()){
        _sendReply(client,462,":You may not reregister");
        return;
    }
    if(args.size() < 5){
        _sendReply(client,461,":Not enough parameters");
        return;
    }
    client->setUsername(args[1]);
    _checkRegistration(client);
}

std::vector<std::string> Server::_splitByComma(const std::string &str) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        result.push_back(item);
    }
    return result;
}

void Server::_cmdJoin(Client *client, const std::vector<std::string> &args)
{
    if (args.size() < 2) {
        _sendReply(client, 461, "JOIN :Not enough parameters");
        return;
    }

    std::vector<std::string> channels = _splitByComma(args[1]);
    std::vector<std::string> keys;
    if (args.size() > 2) {
        keys = _splitByComma(args[2]);
    }

    for (size_t i = 0; i < channels.size(); i++) {
        std::string chanName = channels[i];
        std::string chanKey = (i < keys.size()) ? keys[i] : "";

        if (!_isValidChannelName(chanName)) {
            _sendReply(client, 403, chanName + " :No such channel");
            continue;
        }

        Channel* targetChannel = NULL;
        if (_channels.find(chanName) == _channels.end()) {
            targetChannel = new Channel(chanName);
            targetChannel->addClient(client);
            targetChannel->addOperator(client);
            _channels[chanName] = targetChannel;
        }
        else {
            targetChannel = _channels[chanName];
            
            if(targetChannel->hasClient(client)){
                continue;
            }
            if (targetChannel->hasMode('k')) {
                if (chanKey.empty() || targetChannel->getKey() != chanKey) {
                    _sendReply(client, 475, chanName + " :Cannot join channel (+k)");
                    continue;
                }
            } 
            if (targetChannel->hasMode('l')) {
                if (targetChannel->isFull()) {
                    _sendReply(client, 471, chanName + " :Cannot join channel (+l)");
                    continue;
                }
            }
            if (targetChannel->hasMode('i')) {
                if (!targetChannel->isInvited(client->getNickname())) {
                    _sendReply(client, 473, chanName + " :Cannot join channel (+i)");
                    continue;
                }
            }
            targetChannel->addClient(client);
        }
        targetChannel->broadcast(":" + client->getPrefix() + " JOIN :" + chanName);
        if (targetChannel->getTopic() != "") {
            _sendReply(client, 332, chanName + " :" + targetChannel->getTopic());
        }
        _sendReply(client, 353, "= " + chanName + " :" + targetChannel->getMemberList());
        _sendReply(client, 366, chanName + " :End of /NAMES list");
    }
}

Client* Server::_getClientByNick(const std::string &nickname) 
{
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->second->getNickname() == nickname) {
            return it->second;
        }
    }
    return NULL;
}
void Server::_cmdPrivmsg(Client *client, const std::vector<std::string> &args)
{
    if(args.size() < 3){
        _sendReply(client, 412, ":No text to send");
        return;
    }
    std::string fullMessage = ":" + client->getPrefix() + " PRIVMSG " + args[1] + " :" + args[2];
    if(args[1][0] == '#')
    {
        if(_channels.find(args[1]) != _channels.end())
        {
            Channel* c = _channels[args[1]];
            if(c->hasClient(client)) {
                c->broadcast(fullMessage, client);
                if (args[2] == "!logtimer")
                {
                    _handleBotCommand(client, args);
                }
                return;
            }
            else
            {
                _sendReply(client, 404, args[1] + " :Cannot send to channel");
                return;
            }
        }
        else
        {
             _sendReply(client, 403, args[1] + " :No such channel");
             return;
        }
    }
    else
    {
        Client* target = _getClientByNick(args[1]);
        if(target != NULL)
        {
            target->sendMessage(fullMessage);
            return;
        }
        else
        {
            _sendReply(client, 401, args[1] + " :No such nick/channel");
            return;
        }
    }
}

void Server::_cmdPart(Client *client, const std::vector<std::string> &args)
{
    if(args.size() < 2){
        _sendReply(client, 461, "PART :Not enough parameters");
        return;
    }
    if(_channels.find(args[1]) != _channels.end()){
        Channel* c = _channels[args[1]];
                if(c->hasClient(client)){
                        std::string reason = "";
            if(args.size() > 2) {
                reason = " :" + args[2]; 
            }
            std::string partMsg = ":" + client->getPrefix() + " PART " + args[1] + reason;
            c->broadcast(partMsg); 
            c->removeClient(client);
            c->removeOperator(client);
            if(c->clientCount() == 0){
                delete c;
                _channels.erase(args[1]);
            }
        }
        else{
            _sendReply(client, 442, args[1] + " :You're not on that channel");
        }
    }
    else{
        _sendReply(client, 403, args[1] + " :No such channel");
    }
}

void Server::_cmdKick(Client *client, const std::vector<std::string> &args)
{
    if(args.size() < 3){
        _sendReply(client, 461, "KICK :Not enough parameters");
        return;
    }
        if(_channels.find(args[1]) == _channels.end()){
        _sendReply(client, 403, args[1] + " :No such channel");
        return;
    }
    Channel* c = _channels[args[1]];
    if(!c->hasClient(client)){
        _sendReply(client, 442, args[1] + " :You're not on that channel");
        return;
    }
    if(!c->isOperator(client)){
        _sendReply(client, 482, args[1] + " :You're not channel operator");
        return;
    }
    Client* target = _getClientByNick(args[2]);
    if(!target){
        _sendReply(client, 401, args[2] + " :No such nick/channel");
        return;
    }
    if(!c->hasClient(target)){
        _sendReply(client, 441, args[2] + " " + args[1] + " :They aren't on that channel");
        return;
    }
    std::string reason = " :Kicked by operator";
    if(args.size() > 3){
        reason = " :" + args[3];
    }
    c->broadcast(":" + client->getPrefix() + " KICK " + args[1] + " " + args[2] + reason);
    c->removeClient(target);
    c->removeOperator(target);

    if(c->clientCount() == 0){
        delete c;
        _channels.erase(args[1]);
    }
}

void Server::_cmdTopic(Client *client, const std::vector<std::string> &args)
{
    if(args.size() < 2){
        _sendReply(client, 461, "TOPIC :Not enough parameters");
        return;
    }
        if(_channels.find(args[1]) == _channels.end()){
        _sendReply(client, 403, args[1] + " :No such channel");
        return;
    }
    Channel* c = _channels[args[1]];
    if(!c->hasClient(client)){
         _sendReply(client, 442, args[1] + " :You're not on that channel");
        return;
    }
    if(args.size() == 2){
        const std::string& topic = c->getTopic();
        if(topic == ""){
            _sendReply(client, 331, args[1] + " :No topic is set");
        }
        else{
            _sendReply(client, 332, args[1] + " :" + topic);
        }
        return;
    }
    else{
        if(c->hasMode('t')){
            if(!c->isOperator(client)){
                _sendReply(client, 482, args[1] + " :You're not channel operator");
                return;
            }
        }
        c->setTopic(args[2]);
        c->broadcast(":" + client->getPrefix() + " TOPIC " + args[1] + " :" + args[2]);
    }
}

void Server::_cmdInvite(Client *client, const std::vector<std::string> &args)
{
    if(args.size() < 3) {
        _sendReply(client, 461, "INVITE :Not enough parameters");
        return;
    }
    std::string targetNick = args[1];
    std::string channelName = args[2];

    Client* target = _getClientByNick(targetNick);
    if(!target) {
        _sendReply(client, 401, targetNick + " :No such nick/channel");
        return;
    }
    if(_channels.find(channelName) == _channels.end()) {
        _sendReply(client, 403, channelName + " :No such channel");
        return;
    }

    Channel* c = _channels[channelName];
    if(!c->hasClient(client)) {
        _sendReply(client, 442, channelName + " :You're not on that channel");
        return;
    }

    if(c->hasClient(target)) {
        _sendReply(client, 443, targetNick + " " + channelName + " :is already on channel");
        return;
    }

    if(c->hasMode('t')) {
        if(!c->isOperator(client)) {
            _sendReply(client, 482, channelName + " :You're not channel operator");
            return;
        }
    }

    c->addInvite(targetNick);
    _sendReply(client, 341, channelName + " " + targetNick);
    std::string inviteMsg = ":" + client->getPrefix() + " INVITE " + targetNick + " :" + channelName;
    target->sendMessage(inviteMsg);
}

void Server::_cmdMode(Client *client, const std::vector<std::string> &args)
{
    if (args.size() < 2) {
        _sendReply(client, 461, "MODE :Not enough parameters");
        return;
    }
    if (args[1][0] != '#') {
        return;
    }
    if (_channels.find(args[1]) == _channels.end()) {
        _sendReply(client, 403, args[1] + " :No such channel");
        return;
    }

    Channel *targetChannel = _channels[args[1]];
    if (args.size() == 2) {
        _sendReply(client, 324, args[1] + " " + targetChannel->modesActive()); 
        return;
    }
    if (!targetChannel->isOperator(client)) {
        _sendReply(client, 482, args[1] + " :You're not channel operator");
        return;
    }

    std::string modeString = args[2];
    bool adding = true;
    size_t argIndex = 3;
    
    std::string appliedModes = "";
    std::string appliedArgs = "";
    char lastSign = '\0';

    for (size_t i = 0; i < modeString.length(); i++) {
        char c = modeString[i];
        
        if (c == '+') {
            adding = true;
        } 
        else if (c == '-') {
            adding = false;
        } 
        else if (c == 'i' || c == 't') {
            if (adding == true) {
                if (!targetChannel->hasMode(c)) {
                    targetChannel->setMode(c);
                    if (lastSign != '+') { appliedModes += "+"; lastSign = '+'; }
                    appliedModes += c;
                }
            } else {
                if (targetChannel->hasMode(c)) {
                    targetChannel->unsetMode(c);
                    if (lastSign != '-') { appliedModes += "-"; lastSign = '-'; }
                    appliedModes += c;
                }
            }
        }
        else if (c == 'k') {
            if (adding == true) {
                if (argIndex < args.size()) {
                    targetChannel->setKey(args[argIndex]);
                    targetChannel->setMode(c);
                    if (lastSign != '+') { appliedModes += "+"; lastSign = '+'; }
                    appliedModes += c;
                    appliedArgs += " " + args[argIndex];
                    argIndex++;
                }
                else {
                    _sendReply(client, 461, "MODE :Not enough parameters : +k");
                }
            } else {
                targetChannel->unsetMode('k');
                if (lastSign != '-') { appliedModes += "-"; lastSign = '-'; }
                appliedModes += c;
                if (argIndex < args.size()) { argIndex++; }
            }
        } 
        else if (c == 'l') {
            if (adding == true) {
                if (argIndex < args.size()) {
                    int limit = atoi(args[argIndex].c_str());
                    if (limit > 0) {
                        targetChannel->setUserLimit(limit);
                        targetChannel->setMode(c);
                        if (lastSign != '+') { appliedModes += "+"; lastSign = '+'; }
                        appliedModes += c;
                        appliedArgs += " " + args[argIndex];
                    }
                    argIndex++;
                }
                else {
                    _sendReply(client, 461, "MODE :Not enough parameters : +l");
                }
            } 
            else
            {
                targetChannel->unsetMode('l');
                if (lastSign != '-') { appliedModes += "-"; lastSign = '-'; }
                appliedModes += c;
            }
        } 
        else if (c == 'o') {
            if (argIndex < args.size()) {
                std::string targetNick = args[argIndex];
                Client *targetUser = _getClientByNick(targetNick);
                
                if (targetUser != NULL && targetChannel->hasClient(targetUser)) {
                    if (adding == true) {
                        targetChannel->addOperator(targetUser);
                        if (lastSign != '+') { appliedModes += "+"; lastSign = '+'; }
                    } else {
                        targetChannel->removeOperator(targetUser);
                        if (lastSign != '-') { appliedModes += "-"; lastSign = '-'; }
                    }
                    appliedModes += c;
                    appliedArgs += " " + targetNick;
                } else {
                    _sendReply(client, 441, targetNick + " " + args[1] + " :They aren't on that channel");
                }
                argIndex++;
            }
            else {
                _sendReply(client, 461, "MODE :Not enough parameters : +o");
            }
        }
        else {
            std::string unknownChar(1, c);
            _sendReply(client, 472, unknownChar + " :is unknown mode char to me for " + args[1]);
        }
    }

    if (!appliedModes.empty()) {
        targetChannel->broadcast(":" + client->getPrefix() + " MODE " + args[1] + " " + appliedModes + appliedArgs);
    }
}

void Server::_cmdQuit(Client *client, const std::vector<std::string> &args)
{
    std::string message = "Quit";
    if (args.size() > 1) {
        message = args[1];
    }
    std::map<std::string, Channel*> channelsCopy = _channels;
    for (std::map<std::string, Channel*>::iterator it = channelsCopy.begin(); it != channelsCopy.end(); ++it) {
        Channel* c = it->second;
        
        if (c->hasClient(client)) {
            c->broadcast(":" + client->getPrefix() + " QUIT :" + message, client); 
            c->removeClient(client);
            c->removeOperator(client);
            if (c->clientCount() == 0) {
                delete c;
                _channels.erase(it->first);
            }
        }
    }
    _disconnectClient(client->getFd());
}

void Server::cleaning()
{    
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        Client *client = it->second;
        std::string msg = "ERROR :Server shutting down\r\n";
        send(client->getFd(), msg.c_str(), msg.length(), MSG_NOSIGNAL);
        close(client->getFd());
        delete client;
    }
    _clients.clear();
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        delete it->second;
    }
    _channels.clear();
    if (_serverFd != -1) {
        close(_serverFd);
    }
    _pollfds.clear();
}

void Server::setupSignalHandler()
{
    g_server = this;
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGQUIT, signalHandler);
}

void Server::_cmdPing(Client *client, const std::vector<std::string> &args)
{
    if (args.size() < 2) {
        _sendReply(client, 409, ":No origin specified");
        return;
    }
    std::string pongMsg = ":" + std::string("ft_irc") + " PONG " + std::string("ft_irc") + " :" + args[1] + "\r\n";
    send(client->getFd(), pongMsg.c_str(), pongMsg.length(), MSG_NOSIGNAL);
}

bool Server::_isValidNickname(const std::string &nick) {
    if (nick.empty() || nick.length() > 30) {
        return false;
    }
    if (std::isdigit(nick[0])) {
        return false;
    }
    for (size_t i = 0; i < nick.length(); i++) {
        char c = nick[i];
        if (!std::isalnum(c) && c != '_' && c != '-' && c != '[' && c != ']' && c != '\\' && c != '^' && c != '{' && c != '}') {
            return false;
        }
    }
    return true;
}
bool Server::_isValidChannelName(const std::string &name) {
    if (name.empty() || name.length() > 50 || name[0] != '#' || name.length() <= 1) {
        return false;
    }
        for (size_t i = 0; i < name.length(); i++) {
        if (name[i] == ' ' || name[i] == ',' || name[i] == 7) {
            return false;
        }
    }
    
    return true;
}

void Server::_handleBotCommand(Client *client, const std::vector<std::string> &args)
{
    std::string response;
    
    std::string target = args[1];

    std::string botPrefix = ":47bot!bot@localhost PRIVMSG " + target;
    if (args.size() > 2 && args[2] == "!logtimer") {
        std::time_t now = std::time(NULL);
        double diff = std::difftime(now, client->getConnectionTime());
        long totalSeconds = static_cast<long>(diff);
        if (totalSeconds < 0)
            totalSeconds = 0;

        long days = totalSeconds / 86400;
        totalSeconds %= 86400;
        long hours = totalSeconds / 3600;
        totalSeconds %= 3600;
        long minutes = totalSeconds / 60;
        long seconds = totalSeconds % 60;

        std::vector<std::string> parts;
        if (days > 0) {
            std::stringstream ssDays;
            ssDays << days << (days == 1 ? " day" : " days");
            parts.push_back(ssDays.str());
        }
        if (hours > 0) {
            std::stringstream ssHours;
            ssHours << hours << (hours == 1 ? " hour" : " hours");
            parts.push_back(ssHours.str());
        }
        if (minutes > 0) {
            std::stringstream ssMins;
            ssMins << minutes << (minutes == 1 ? " minute" : " minutes");
            parts.push_back(ssMins.str());
        }
        if (seconds > 0 || parts.empty()) {
            std::stringstream ssSecs;
            ssSecs << seconds << (seconds == 1 ? " second" : " seconds");
            parts.push_back(ssSecs.str());
        }

        std::stringstream timeStr;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) {
                if (i == parts.size() - 1)
                    timeStr << " and ";
                else
                    timeStr << ", ";
            }
            timeStr << parts[i];
        }

        std::stringstream ss;
        ss << botPrefix << " :You have been connected for " << timeStr.str();
        response = ss.str();
    }
    
    if (_channels.find(args[1]) != _channels.end()) {
        _channels[args[1]]->broadcast(response);
    }
}