// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// Modified for BMW F10/F11 6WA by WesVoj, 2026. See README.md.
// 
// ####################################################################################################################

#ifndef BEAM_NG_GAME
#define BEAM_NG_GAME

#include "Arduino.h"
#include "AsyncUDP.h" // For game integration (system library part of ESP core)

#include "GameSimulation.h"

class BeamNGGame: public Game {
  public:
    BeamNGGame(GameState& game, int port);
    void begin();

  private:
    int port;
    AsyncUDP beamUdp;
    unsigned long lastExtendedPacketMillis = 0;
    unsigned long lastLeftSignalMillis = 0;
    unsigned long lastRightSignalMillis = 0;
};

#endif
