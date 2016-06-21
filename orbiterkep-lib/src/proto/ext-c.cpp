#include "proto/solution.pb-c.h"
#include "proto/parameters.pb-c.h"

#include "proto/ext-c.h"

#include <keplerian_toolbox/epoch.h>

#include <stdio.h>
#include <iostream>
#include <sstream>

#ifdef __cplusplus
extern "C" {
#endif
int __cdecl sprintf_transx_arrival(char * buf, const Orbiterkep__TransXArrival * arriv) {
    int total_written = 0;

    total_written += sprintf(buf + total_written, "%s arrival\n", arriv->planet);
    total_written += sprintf(buf + total_written, "--------------------------------------\r\n");
    total_written += sprintf(buf + total_written, "MJD:                  %12.4f    \r\n", arriv->mjd);
    total_written += sprintf(buf + total_written, "Hyp. excess velocity: %11.3f m/s\r\n", arriv->vinf);
    total_written += sprintf(buf + total_written, "Orbit insertion burn: %11.3f m/s\r\n", arriv->burn);
    total_written += sprintf(buf + total_written, "\r\n\r\n");

    return total_written;
}

int __cdecl sprintf_transx_solution(char * buf, const Orbiterkep__TransXSolution * sol) {
    int total_written = 0;
    total_written += sprintf_transx_times(buf + total_written, sol->times);
    total_written += sprintf_transx_escape(buf + total_written, sol->escape);

    int i = 0;
    int j = 0;
    Orbiterkep__TransXDSM ** dsms = sol->dsms;
    Orbiterkep__TransXFlyBy ** flybyes = sol->flybyes;
    while (i < sol->n_dsms || j < sol->n_flybyes) {
        if (i < sol->n_dsms) {
            if (sol->dsms[i]->leg == j) {
                total_written += sprintf_transx_dsm(buf + total_written, sol->dsms[i]);
                i += 1;
            }
        }
        if (j < sol->n_flybyes) {
            total_written += sprintf_transx_flyby(buf + total_written, sol->flybyes[j]);
            j += 1;
        }
    }
    total_written += sprintf_transx_arrival(buf + total_written, sol->arrival);
    total_written += sprintf(buf + total_written, "Total delta-V:        %11.3f m/s\r\n", sol->fuel_cost);

    return total_written;
}

int __cdecl sprintf_transx_times(char * buf, const Orbiterkep__TransXTimes * times) {
    int total_written = 0;

    int i;
    for (i = 0; i < times->n_planets - 1; ++i) {
        total_written += sprintf(buf + total_written, "Transfer time from %s to %s:", times->planets[i], times->planets[i + 1]);
        total_written += sprintf(buf + total_written, "%0.2f\r\n", times->times[i + 1] - times->times[i]);
    }

    for (i = 0; i < times->n_planets; ++i) {
        std::stringstream ss;
        ss << kep_toolbox::epoch(times->times[i], kep_toolbox::epoch::type::MJD);
        total_written += sprintf(buf + total_written, "Date of %s encounter:", times->planets[i]);
        total_written += sprintf(buf + total_written, "%s\r\n", ss.str().c_str());
    }

    total_written += sprintf(buf + total_written, "Total mission duration: %0.2f days\n", times->times[times->n_times - 1] - times->times[0]);
    total_written += sprintf(buf + total_written, "\r\n\r\n");
    return total_written;
}

int __cdecl sprintf_transx_escape(char * buf, const Orbiterkep__TransXEscape * escape) {
    int total_written = 0;

    total_written += sprintf(buf + total_written, "Escape - %s\n", escape->planet);
    total_written += sprintf(buf + total_written, "--------------------------------------\r\n");
    total_written += sprintf(buf + total_written, "MJD:                  %12.4f\r\n", escape->mjd);
    total_written += sprintf(buf + total_written, "Prograde:             %11.3f m/s\r\n", escape->prograde);
    total_written += sprintf(buf + total_written, "Outward:              %11.3f m/s\r\n", escape->outward);
    total_written += sprintf(buf + total_written, "Plane:                %11.3f m/s\r\n", escape->plane);
    total_written += sprintf(buf + total_written, "Hyp. excess velocity: %11.3f m/s\r\n", escape->vinf);
    total_written += sprintf(buf + total_written, "Earth escape burn:    %11.3f m/s\r\n", escape->burn);
    total_written += sprintf(buf + total_written, "\r\n\r\n");

    return total_written;
}

int __cdecl sprintf_transx_dsm(char * buf, const Orbiterkep__TransXDSM * dsm) {
    int total_written = 0;

    total_written += sprintf(buf + total_written, "Deep Space Maneuver\n");
    total_written += sprintf(buf + total_written, "--------------------------------------\r\n");
    total_written += sprintf(buf + total_written, "MJD:                  %12.4f\r\n", dsm->mjd);
    total_written += sprintf(buf + total_written, "Prograde:             %11.3f m/s\r\n", dsm->prograde);
    total_written += sprintf(buf + total_written, "Outward:              %11.3f m/s\r\n", dsm->outward);
    total_written += sprintf(buf + total_written, "Plane:                %11.3f m/s\r\n", dsm->plane);
    total_written += sprintf(buf + total_written, "Hyp. excess velocity: %11.3f m/s\r\n", dsm->vinf);
    total_written += sprintf(buf + total_written, "DSM burn:             %11.3f m/s\r\n", dsm->burn);
    total_written += sprintf(buf + total_written, "\r\n\r\n");

    return total_written;
}

int __cdecl sprintf_transx_flyby(char * buf, const Orbiterkep__TransXFlyBy * flyby) {
    int total_written = 0;

    total_written += sprintf(buf + total_written, "%s encounter\n", flyby->planet);
    total_written += sprintf(buf + total_written, "--------------------------------------\r\n");
    total_written += sprintf(buf + total_written, "MJD:                %12.4f\r\n", flyby->mjd);
    total_written += sprintf(buf + total_written, "Approach velocity:  %11.3f m/s\r\n", flyby->approach_vel);
    total_written += sprintf(buf + total_written, "Departure velocity: %11.3f m/s\r\n", flyby->departure_vel);

    total_written += sprintf(buf + total_written, "Outward angle:      %11.3f deg\r\n", flyby->outward_angle);
    total_written += sprintf(buf + total_written, "Inclination:        %11.3f deg\r\n", flyby->inclination);
    total_written += sprintf(buf + total_written, "Turning angle:      %11.3f deg\r\n", flyby->turning_angle);
    total_written += sprintf(buf + total_written, "Periapsis altitude: %11.3f km\r\n", flyby->periapsis_altitude);
    total_written += sprintf(buf + total_written, "dV needed:          %11.3f m/s\r\n", flyby->burn);
    total_written += sprintf(buf + total_written, "\r\n\r\n");

    return total_written;
}

#ifdef __cplusplus
}
#endif
