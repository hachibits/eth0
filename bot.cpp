#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include <string>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <sstream>

class Configuration {
private:
  /*
    0 = prod-like
    1 = slower
    2 = empty
  */
  static int const test_exchange_index = 0;
public:
  std::string team_name;
  std::string exchange_hostname;
  int exchange_port;
  Configuration(bool test_mode) : team_name("ol"){
    exchange_port = 20000;
    if(true == test_mode) {
      exchange_hostname = "test-exch-" + team_name;
      exchange_port += test_exchange_index;
    } else {
      exchange_hostname = "production";
    }
  }
};

class Connection {
private:
  FILE * in;
  FILE * out;
  int socket_fd;
public:
  Connection(Configuration configuration){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      throw std::runtime_error("Could not create socket");
    }
    std::string hostname = configuration.exchange_hostname;
    hostent *record = gethostbyname(hostname.c_str());
    if(!record) {
      throw std::invalid_argument("Could not resolve host '" + hostname + "'");
    }
    in_addr *address = reinterpret_cast<in_addr *>(record->h_addr);
    std::string ip_address = inet_ntoa(*address);
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(ip_address.c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(configuration.exchange_port);

    int res = connect(sock, ((struct sockaddr *) &server), sizeof(server));
    if (res < 0) {
      throw std::runtime_error("could not connect");
    }
    FILE *exchange_in = fdopen(sock, "r");
    if (exchange_in == NULL){
      throw std::runtime_error("could not open socket for writing");
    }
    FILE *exchange_out = fdopen(sock, "w");
    if (exchange_out == NULL){
      throw std::runtime_error("could not open socket for reading");
    }

    setlinebuf(exchange_in);
    setlinebuf(exchange_out);
    this->in = exchange_in;
    this->out = exchange_out;
    this->socket_fd = res;
  }

  void send_to_exchange(std::string input) {
    std::string line(input);
    std::transform(line.begin(), line.end(), line.begin(), ::toupper);
    int res = fprintf(this->out, "%s\n", line.c_str());
    if (res < 0) {
      throw std::runtime_error("error sending to exchange");
    }
  }

  std::string read_from_exchange()
  {
    const size_t len = 10000;
    char buf[len];
    if(!fgets(buf, len, this->in)){
      throw std::runtime_error("reading line from socket");
    }

    int read_length = strlen(buf);
    std::string result(buf);
    result.resize(result.length() - 1);
    return result;
  }
};

std::string join(std::string sep, std::vector<std::string> strs) {
  std::ostringstream stream;
  const int size = strs.size();
  for(int i = 0; i < size; ++i) {
    stream << strs[i];
    if(i != (strs.size() - 1)) {
      stream << sep;
    }
  }
  return stream.str();
}


int main(int argc, char *argv[])
{
    bool test_mode = true;
    Configuration config(test_mode);
    Connection conn(config);

    std::vector<std::string> data;
    data.push_back(std::string("Connection established."));
    data.push_back(config.team_name);
    conn.send_to_exchange(join(" ", data));
    std::string line = conn.read_from_exchange();
    std::cout << "The exchange replied: " << line << std::endl;
    while(true) {
      std::string message = conn.read_from_exchange();
      if(std::string(message).find("CLOSE") == 0) {
        std::cout << "The round has ended" << std::endl;
        break;
      }
    }
    return 0;
}
