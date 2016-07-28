
// main.cpp

// includes

#include <cstdio>
#include <cstdlib>

#include "attack.h"
#include "book.h"
#include "hash.h"
#include "move_do.h"
#include "option.h"
#include "pawn.h"
#include "piece.h"
#include "protocol.h"
#include "random.h"
#include "square.h"
#include "trans.h"
#include "util.h"
#include "value.h"
#include "vector.h"
#include "probe.h"

#pragma warning( disable : 4800 )
// Level 2 warning C4800: 'int' : forcing value to bool 'true' or 'false' (performance warning)

static char * egbb_path = "c:/egbb/";
static uint32 egbb_cache_size = 16; 

// functions

// main()

int main(int argc, char * argv[]) {

   // init

   util_init();
   my_random_init(); // for opening book

   printf("Toga II 2.0 SE\n");
   printf("\n");
   printf("based on:\n");
   printf("Fruit 2.1 Fabien Letouzey and \n");
   printf("Toga II 1.4 Beta5c by Thomas Gaksch\n");
   printf("Toga II 1.4.1SE by Chris Formula\n");
   printf("created by Kranium\n");
   printf("powered by Kiro\n");
   printf("\n");

   option_init();
   square_init();
   piece_init();
   pawn_init_bit();
   value_init();
   vector_init();
   attack_init();
   move_do_init();

   random_init();
   hash_init();

   trans_init(Trans);
   book_init();
   init_threads();
   egbb_cache_size = (egbb_cache_size * 1024 * 1024);
   egbb_is_loaded = LoadEgbbLibrary(egbb_path,egbb_cache_size);
   if (!egbb_is_loaded)
		printf("EgbbProbe not Loaded!\n");

   // loop

   loop();

   return EXIT_SUCCESS;
}

// end of main.cpp

