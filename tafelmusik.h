#include <iostream>
#include <string>

/*!
 * "Tafelmusik" is a simple interface for playing music from C++ program.
 * Currently the only implemented player is TinySID for Commodore 64 SID
 * music.
 *
 * \author Mikael Lund
 * \date Malmo, 2012
 */
namespace Tafelmusik {
  extern "C" {
    #include "tinysid.h"
  }

  class Playerbase {
    public:
      virtual bool init(std::string)=0;
      virtual void play()=0;
      virtual void stop()=0;
      virtual void setSubsong(unsigned char)=0;
  };

  class TinySIDPlayer : public Playerbase {
    private:
      unsigned short init_addr, play_addr;
      unsigned char actual_subsong, max_subsong, play_speed;
      char song_name[32], song_author[32], song_copyright[32];
      char szFilename[1024];
    public:
      bool init(std::string);
      void play();
      void stop();
      void setSubsong(unsigned char);
      std::string getAuthor();
      std::string getTitle();
      std::string getCopyright();
  };
} //namespace

