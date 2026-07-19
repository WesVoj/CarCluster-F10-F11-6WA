// ####################################################################################################################
//
// Spotify / Voicemeeter audio VU test page for CarCluster.
//
// ####################################################################################################################

#ifndef AUDIO_VU_WEB_PAGE_H
#define AUDIO_VU_WEB_PAGE_H

#include "HttpWebServer.h"

void webDashboardAudioVU(HttpConnection *connection, int event, void *eventData);

#endif
