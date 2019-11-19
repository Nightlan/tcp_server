#include <iostream>
#include <tcp_server.h>

int main(int argc, char* argv[])
{
  std::shared_ptr<TCPServer> server(new TCPServer());
  if (server->InitServer("0.0.0.0", 8888, 2) != 0) {
    std::cout << "init server failed!" << std::endl;
    return 0;
  }
  if (server->StartServer() != 0) {
    std::cout << "start server failed!" << std::endl;
    return 0;
  }
  std::string line;
  std::cout << "input q exit." << std::endl;
  while (true) {
    std::getline(std::cin, line);
    if (line == "q") {
      break;
    }
  }
  server->StopServer();
  return 0;
}