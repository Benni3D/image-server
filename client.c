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
   if (argc != 2) {
      fputs("Usage: client <address>\n", stderr);
      return 1;
   }

   struct sockaddr_in server_addr;
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(5555);
   server_addr.sin_addr.s_addr = inet_addr(argv[1]);
   if (server_addr.sin_addr.s_addr == 0xffffffff) {
      error("'%s' is not a valid IPv4 address", argv[1]);
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

   if (send(sock, buffer, len_buffer, 0) != len_buffer) {
      error("failed to send image data: %s", geterr());
      free(buffer);
      close(sock);
      return 1;
   }

   free(buffer);
   close(sock);
   return 0;
}
