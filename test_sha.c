#include <stdio.h>
#include "sha1_min.h"


int main()
{
  struct Context ctx;
  unsigned char hash[20] ="12345678901234567890";
  unsigned char buffer[1024];
  int count=0;

  sha1_init(&ctx, hash);
  while (0<(count = read(0, buffer, sizeof(buffer))))
    sha1(&ctx, buffer, count, 0, 0);
  sha1_finish(&ctx);

  for (unsigned i=0; i<20; i++)
    printf("%02x", hash[i]);
  printf(" - \n");
}
