// ####################################################################################################################
// 
// Code part of CarCluster project by Andrej Rolih. See .ino file more details
// Modified for BMW F10/F11 6WA by WesVoj, 2026. See README.md.
// 
// ####################################################################################################################

#ifndef WEB_DASHBOARD
#define WEB_DASHBOARD

#include "Arduino.h"

#include "HttpWebServer.h"

#include "../Games/GameSimulation.h"


class WebDashboard {
  WebDashboard(const WebDashboard &other) = delete;
  WebDashboard(WebDashboard &&other) = delete;
  WebDashboard &operator=(const WebDashboard &other) = delete;
  WebDashboard &operator=(WebDashboard &&other) = delete;

  public:
    WebDashboard(GameState& game, unsigned long webDashboardUpdateInterval);
    void update();
    void getState(DashboardState *data);
    void setState(DashboardState *data);
    void steeringWheelAction(HttpString params);

  private:
    GameState &gameState;
    unsigned long webDashboardUpdateInterval;
    unsigned long lastWebDashboardUpdateTime = 0;

    const char* mapGenericGearToLocalGear(GearState inputGear);
    GearState mapLocalGearToGenericGear(const char *gear);
    const char* mapGenericDriveModeToLocalDriveMode(uint8_t driveMode);
    uint8_t mapLocalDriveModeToGenericDriveMode(const char *driveMode);
};

#endif
