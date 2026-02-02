#include <stdio.h>
#include <string.h>
#include "../include/strlwr.h"

char *strlwr(char *str){
    char *ptr = str;
    while (*ptr) {
        if (*ptr >= 'A' && *ptr <= 'Z'){
            *ptr = *ptr + 32;
        }
        ptr++;
    }
    return str;
}


/* #include <stdio.h>

  int main() {
      printf("A = %d\n", 'A');   // 65
      printf("Z = %d\n", 'Z');   // 90
      printf("a = %d\n", 'a');   // 97
      printf("z = %d\n", 'z');   // 122

      char letra = 'A';
      printf("%c + 32 = %c\n", letra, letra + 32);  // A + 32 = a

      return 0;
  }
 *
 *
 *
 * */

