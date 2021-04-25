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

struct net_context {
   SDL_Surface* surface;
   uint16_t port;
};

static atomic_bool running;
static atomic_bool update_screen;

static void* thread_net(void* arg) {
   struct net_context* ctx = arg;
   SDL_Surface* surface = ctx->surface;
   const int server = socket(AF_INET, SOCK_STREAM, 0);
   if (server < 0) {
      error("failed to create socket: %s", geterr());
      exit(1);
   }

   struct sockaddr_in server_addr;
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(ctx->port);
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
      size_t recvd = 0;
      while (recvd < num_bytes) {
         if (!atomic_load(&running)) { close(client); goto end; }
         const ssize_t tmp = recv(client, (void*)((size_t)surface->pixels + recvd), num_bytes - recvd, 0);
         if (tmp < 0) {
            error("failed to receive image data: %s", geterr());
            close(client);
            continue;
         }
         recvd += tmp;
      }
      atomic_store(&update_screen, 1);
      close(client);
      sleep(1);
   }
end:
   close(server);
   return NULL;
}

int main(int argc, char* argv[]) {
   struct net_context ctx;
   char* endp;
   atomic_init(&running, 0);
   atomic_init(&update_screen, 1);
   ctx.port = 5555;
  
   int option;
   while ((option = getopt(argc, argv, "p:")) != -1) {
      switch (option) {
      case 'p': 
         ctx.port = (uint16_t)strtoul(optarg, &endp, 10);
         if (endp && *endp) {
            error("'%s' is not a valid port", optarg);
            return 1;
         }
         break;
      default: goto print_usage;
      }
   }

   // Parse parameters
   if ((argc - optind) != 2) {
   print_usage:
      fputs("Usage: server [-p port] <width> <height>\n", stderr);
      return 1;
   }
   int width, height;
   width = (int)strtoul(argv[optind], &endp, 10);
   if (endp && *endp) {
      error("'%s' is not a valid number", argv[optind]);
      return 1;
   }
   height = (int)strtoul(argv[optind + 1], &endp, 10);
   if (endp && *endp) {
      error("'%s' is not a valid number", argv[optind + 1]);
      return 1;
   }

   // Initialize display
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
   ctx.surface = surface;

   // Start loop
   atomic_store(&running, 1);
   pthread_t thr;
   if (pthread_create(&thr, NULL, thread_net, &ctx) != 0) {
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

      if (atomic_load(&update_screen)) {
         SDL_BlitScaled(surface, NULL, SDL_GetWindowSurface(window), NULL);
         SDL_UpdateWindowSurface(window);
         atomic_store(&update_screen, 0);
      }
      usleep((unsigned)(1000.0f / 30.0f));
   }
   pthread_join(thr, NULL);
   
   SDL_FreeSurface(surface);
   SDL_DestroyWindow(window);
   SDL_Quit();
   return 0;
}











