/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 * 
 * 
 * Copyright (c) 2016, Lutz, Clemens <lutzcle@cml.li>
 */

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <string>

#include <unistd.h> // close, getopt
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "timer.hpp"
#include "statistics.hpp"

#define SERVER_IP "127.0.0.1"
#define PORT 3030
#define MSG_LEN 512
#define NUM_MSGS 100000

class ParseArgs {
public:
  void parse(int argc, char **argv) {
    int opt = 0;

    while ((opt = getopt(argc, argv, "c:sp:l:n:o:")) != -1) {
      switch (opt) {
      case 'c':
        is_client_ = true;
	server_ip_ = optarg;
	break;
      case 's':
	is_client_ = false;
	break;
      case 'p':
	port_ = atoi(optarg);
	break;
      case 'l':
	msg_len_ = atoi(optarg);
	break;
      case 'n':
	num_msgs_ = atoi(optarg);
	break;
      case 'o':
	optimization_ = atoi(optarg);
	break;
      default:
        std::cerr << "Usage: " << argv[0] << " [-c server_ip] [-p port] [-l msg_len] [-n num_msgs] [-o optimization]" << std::endl;
        exit(EXIT_FAILURE);
      }
    }

  }

  bool is_client() { return is_client_; } 
  std::string server_ip() { return server_ip_; }
  short port() { return port_; }
  int msg_len() { return msg_len_; }
  int num_msgs() { return num_msgs_; }
  int optimization() { return optimization_; }

private:
  bool is_client_ = true;
  std::string server_ip_ = SERVER_IP;
  short port_ = PORT;
  int msg_len_ = MSG_LEN;
  int num_msgs_ = NUM_MSGS;
  int optimization_ = 0;
};

class PingUDP {
public:
  PingUDP(int optimization = 0) : optimization(optimization) {}

  int start_client(std::string server_ip, short port, int msg_len, int num_msgs)
  {
    int ret = 0;
    int sock = 0;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
      perror("Socket not opened");
      return EXIT_FAILURE;
    }

    struct sockaddr_in sock_other = {0};
    sock_other.sin_family = AF_INET;
    sock_other.sin_port = htons(port);

    ret = inet_aton(server_ip.c_str(), &sock_other.sin_addr);
    if (ret == 0) {
      std::cerr << "Invalid server IP address" << std::endl;
      close(sock);
      return EXIT_FAILURE;
    }

    int recv_len = 0;
    socklen_t sockaddr_len = sizeof(sock_other);
    char message[msg_len];
    char buffer[msg_len];
    Timer::Timer rtt_timer;
    uint64_t rtt_ns;
    Statistics::Statistics stats;

    for (int i = 0; i < num_msgs; ++i) {

      rtt_timer.start();

      ret = sendto(sock, message, msg_len, 0, (struct sockaddr *)&sock_other, sockaddr_len);
      if (ret == -1) {
        perror("Sendto failed");
	close(sock);
        return EXIT_FAILURE;
      }

      switch (optimization) {
      case 0:
        recv_len = recvfrom(sock, buffer, msg_len, 0, (struct sockaddr *)&sock_other, &sockaddr_len);
	break;
      case 2:
	do {
          recv_len = recvfrom(sock, buffer, msg_len, MSG_DONTWAIT, (struct sockaddr *)&sock_other, &sockaddr_len);
	} while (recv_len == -1 and (errno == EAGAIN or errno == EWOULDBLOCK));
	break;
      }

      if (recv_len == -1) {
        perror("Receive packet failed");
	close(sock);
        return EXIT_FAILURE;
      }
    
      rtt_ns = rtt_timer.stop();
      stats.update(rtt_ns);
    }

    std::cout << "Mean: " << stats.mean() / 1000 << " us"
      << " Variance: " << stats.variance() / 1000 / 1000 << " us"
      << " Min: " << stats.min() / 1000 << " us"
      << " Max: " << stats.max() / 1000 << " us"
      << std::endl;

    close(sock);
    
    return EXIT_SUCCESS;
  }

  int start_server(short port, int buf_len) {
    int ret = 0;
    int sock = 0;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
      perror("Socket not opened");
      return EXIT_FAILURE;
    }

    struct sockaddr_in sock_in = {0};
    sock_in.sin_family = AF_INET;
    sock_in.sin_port = htons(port);
    sock_in.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = bind(sock, (struct sockaddr *)&sock_in, sizeof(sock_in));
    if (ret == -1) {
      perror("Couldn't bind to socket");
      close(sock);
      return EXIT_FAILURE;
    }

    int recv_len = 0;
    struct sockaddr_in sock_other = {0};
    socklen_t sockaddr_len = sizeof(sock_other);
    char buffer[buf_len];

    while (true) {

      switch (optimization) {
      case 0:
        recv_len = recvfrom(sock, buffer, buf_len, 0, (struct sockaddr *)&sock_other, &sockaddr_len);
	break;
      case 2:
	do {
          recv_len = recvfrom(sock, buffer, buf_len, MSG_DONTWAIT, (struct sockaddr *)&sock_other, &sockaddr_len);
	} while (recv_len == -1 and (errno == EAGAIN or errno == EWOULDBLOCK));
	break;
      }

      if (recv_len == -1) {
        perror("Receive packet failed");
	close(sock);
        return EXIT_FAILURE;
      }

      ret = sendto(sock, buffer, recv_len, 0, (struct sockaddr *)&sock_other, sockaddr_len);
      if (ret == -1) {
        perror("Sendto failed");
	close(sock);
        return EXIT_FAILURE;
      }
    }

    close(sock);

    return EXIT_SUCCESS;
  }

private:
  int optimization;

};

int main(int argc, char **argv) {

  int status = 0;

  ParseArgs args;
  args.parse(argc, argv);

  PingUDP ping_udp(args.optimization());
  if (args.is_client()) {
    status = ping_udp.start_client(args.server_ip(), args.port(), args.msg_len(), args.num_msgs());
  }
  else {
    status = ping_udp.start_server(args.port(), args.msg_len());
  }

  exit(status);
}

