// DQ (1/19/2004): test code showing bug!
// This code demonstrates a problem with how the SgInitialized name fixup does not work properly.

#warning "This is a bug, but I have commented it out!"
// typedef struct _IO_FILE __FILE;

struct _IO_FILE
   {
     int _flags;
   };

typedef struct X
   {
     int x;
   } X;


