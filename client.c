#define PROG "client"
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "common.h"

static int read_random(void* buffer, size_t num) {
   const int fd = open("/dev/urandom", O_RDONLY);
   if (fd < 0) return -1;
   const int success = read(fd, buffer, num) == num;
   close(fd);
   if (!success) errno = EIO;
   return success;
}

int main(int argc, char* argv[]) {
   uint16_t port = 5555;
   int option;
   while ((option = getopt(argc, argv, ":p:")) != -1) {
      char* endp;
      switch (option) {
      case 'p':
         port = (uint16_t)strtoul(optarg, &endp, 10);
         if (*endp) {
            error("'%s' is not a valid port number", optarg);
            return 1;
         }
         break;
      default: goto print_usage;
      }
   }

   if ((argc - optind) != 1) {
   print_usage:
      fputs("Usage: client [-p port] <address>\n", stderr);
      return 1;
   }

   struct sockaddr_in server_addr;
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(port);
   server_addr.sin_addr.s_addr = inet_addr(argv[optind]);
   if (server_addr.sin_addr.s_addr == 0xffffffff) {
      error("'%s' is not a valid IPv4 address", argv[optind]);
      return 1;
   }

   const int sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sock < 0) {
      error("failed to create socket: %s", geterr());
      return 1;
   }

   if (connect(sock, (const struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
      error("failed to connect to server: %s", geterr());
      close(sock);
      return 1;
   }

   struct login_packet login;
   if (recv(sock, &login, sizeof(login), 0) != sizeof(login)) {
      error("failed to receive server information: %s", geterr());
      close(sock);
      return 1;
   }

   const size_t len_buffer = login.width * login.height;
   uint8_t* buffer = malloc(len_buffer);
   if (!buffer) {
      error("failed to allocate buffer: %s", geterr());
      close(sock);
      return 1;
   }
   
   read_random(buffer, len_buffer);
   
   size_t sent = 0;
   while (sent < len_buffer) {
      const ssize_t tmp = send(sock, buffer + sent, len_buffer - sent, 0);
      if (tmp < 0) {
         error("failed to send image data: %s", geterr());
         free(buffer);
         close(sock);
         return 1;
      }
      sent += tmp;
   }

   free(buffer);
   close(sock);
   return 0;
}
