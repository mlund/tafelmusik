#include <iostream>
#include "tafelmusik.h"
int main(int argc, char* argv[]) {
  Tafelmusik::TinySIDPlayer tiny;
  if ( argc>=2 )
    if ( tiny.init(argv[1]) ) {
      if (argc==3 )
        tiny.setSubsong( atoi(argv[2]) );
      tiny.play();
      std::cout << tiny.getTitle() + " - " + tiny.getAuthor() + "\n";
      int wait;
      std::cin >> wait;
      tiny.stop();
    }
}
