#include <stdarg.h>
#include <stdio.h>

#define ABC(xyz) ((xyz)+(xyz))
#define DEF(xyz) ((xyz) \
        +(xyz))

void printMessage(FILE *f, const char *format, ...) {
   va_list ap;

   va_list ap_2;
   va_copy(ap,ap_2);

   va_start(ap, format);
   vfprintf(f, format, ap);
   va_end(ap);
}

int main()
   {
     return 0;
   }

