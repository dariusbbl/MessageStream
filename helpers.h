#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <cstdint> 
#define CONTENT_LEN 1500

// definim macro-ul DIE
#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#endif

// structura pentru mesajul de tip UDP
struct udp_message {
    char topic[50];
    char type;
    char content[CONTENT_LEN + 1];
};

// structura pentru mesajul de tip TCP
struct tcp_message {
    char source_ip[16];
    uint16_t source_port;
    char topic[51];
    char type[50];
    char content[CONTENT_LEN + 1];
};
