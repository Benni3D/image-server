#define PROG "server"
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdatomic.h>
#include <SDL2/SDL.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include "common.h"

static atomic_bool running;

static void* thread_net(void* arg) {
   SDL_Surface* surface = arg;
   const int server = socket(AF_INET, SOCK_STREAM, 0);
   if (server < 0) {
      error("failed to create socket: %s", geterr());
      exit(1);
   }

   struct sockaddr_in server_addr;
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(5555);
   server_addr.sin_addr.s_addr = INADDR_ANY;
   if (bind(server, (const struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
      error("failed to bind address to socket: %s", geterr());
      close(server);
      exit(1);
   }

   while (atomic_load(&running)) {
      if (listen(server, 5) != 0) {
         error("failed to listen for client: %s", geterr());
         continue;
      }
      const int client = accept(server, NULL, NULL);
      if (client < 0) {
         error("failed to accept client: %s", geterr());
         continue;
      }
      struct login_packet login;
      login.width = surface->w;
      login.height = surface->h;
      if (send(client, &login, sizeof(login), 0) != sizeof(login)) {
         error("failed to inform client: %s", geterr());
         close(client);
         continue;
      }
      const size_t num_bytes = surface->w * surface->h;
      if (recv(client, surface->pixels, num_bytes, 0) == -1) {
         error("failed to receive image: %s", geterr());
      }
      close(client);
      continue;
   }

   return NULL;
}

int main(int argc, char* argv[]) {
   atomic_init(&running, 0);
   if (argc != 3) {
      fputs("Usage: server <width> <height>\n", stderr);
      return 1;
   }
   char* endp;
   int width, height;
   width = (int)strtoul(argv[1], &endp, 10);
   if (*endp) {
      error("'%s' is not a valid number", argv[1]);
      return 1;
   }
   height = (int)strtoul(argv[2], &endp, 10);
   if (*endp) {
      error("'%s' is not a valid number", argv[2]);
      return 1;
   }

   if (SDL_Init(SDL_INIT_VIDEO) != 0) {
      error("failed to initialize SDL2: %s", SDL_GetError());
      return 1;
   }
   SDL_Window* window = SDL_CreateWindow(
      "Image Server",
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      width, height,
      SDL_WINDOW_SHOWN
   );
   if (!window) {
      error("failed to create window: %s", SDL_GetError());
      SDL_Quit();
      return 1;
   }
   SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 8, SDL_PIXELFORMAT_RGB332);
   if (!surface) {
      error("failed to create surface: %s", SDL_GetError());
      SDL_DestroyWindow(window);
      SDL_Quit();
      return 1;
   }

   atomic_store(&running, 1);
   pthread_t thr;
   if (pthread_create(&thr, NULL, thread_net, surface) != 0) {
      error("failed to create thread: %s", SDL_GetError());
      SDL_FreeSurface(surface);
      SDL_DestroyWindow(window);
      SDL_Quit();
      return 1;
   }

   while (atomic_load(&running)) {
      SDL_Event e;
      if (SDL_PollEvent(&e) && e.type == SDL_QUIT) {
         atomic_store(&running, 0);
         pthread_kill(thr, SIGTERM);
      }

      SDL_BlitScaled(surface, NULL, SDL_GetWindowSurface(window), NULL);
      SDL_UpdateWindowSurface(window);
   }
   pthread_join(thr, NULL);
   
   SDL_FreeSurface(surface);
   SDL_DestroyWindow(window);
   SDL_Quit();
   return 0;
}











