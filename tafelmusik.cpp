#include "tafelmusik.h"

namespace Tafelmusik {

  bool TinySIDPlayer::init(std::string filename) {
    c64Init();
    synth_init(44100);
    if (c64SidLoad(&filename[0],
          &init_addr,
          &play_addr,
          &actual_subsong,
          &max_subsong,
          &play_speed,
          song_name,
          song_author,
          song_copyright) == 0) {
      return false;
    }
    return true;
  }

  void TinySIDPlayer::play() {
    soundcard_init();
    cpuJSR(init_addr, actual_subsong);
    start_playing(play_addr, play_speed);
  }

  void TinySIDPlayer::stop() {
    stop_playing();
  }

  void TinySIDPlayer::setSubsong(unsigned char sub) {
    if (sub>=0 && sub<=max_subsong)
      actual_subsong=sub;
  }

  std::string TinySIDPlayer::getAuthor() { return song_author; }

  std::string TinySIDPlayer::getTitle() { return song_name; }

  std::string TinySIDPlayer::getCopyright() { return song_copyright; }

} //namespace

