#include "Server.hpp"

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }

    std::srand(std::time(NULL));

    int port = std::atoi(argv[1]);
    std::string password = argv[2];
    Server server(port, password);
    try
    {
        server.setupSignalHandler();
        server.start();
        server.run();
        server.cleaning();
    }
    catch (const std::exception &e)
    {
        server.cleaning();
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    return 0;
}
