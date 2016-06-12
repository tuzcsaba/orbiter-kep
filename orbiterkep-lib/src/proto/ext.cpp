#include "proto/ext.h"

#include <iostream>
#include <iomanip>

namespace orbiterkep {

std::ostream& __cdecl operator<<(std::ostream& ss, const TransXArrival &arriv) {

  ss << std::fixed;
  ss << arriv.planet() << " arrival" << std::endl;
  ss << "--------------------------------------" << std::endl;
  ss << "MJD:                     " << arriv.mjd() << std::setprecision(4) << "   " << std::endl;
  ss << "Hyp. excess velocity:    " << arriv.vinf() << std::setprecision(3) << " m/s" << std::endl;
  ss << "Orbit insertion burn:    " << arriv.burn() << std::setprecision(3) << " m/s" << std::endl;
  ss << std::endl << std::endl;

  return ss;
}

std::ostream& __cdecl operator<<(std::ostream& ss, const TransXSolution &sol)
{
  ss << sol.times();

  ss << sol.escape();

  int i = 0;
  int j = 0;
  auto dsms = sol.dsms();
  auto flybyes = sol.flybyes();
  while (i < dsms.size() || j < flybyes.size()) {
    if (i < dsms.size()) {
      ss << dsms.Get(i++);
    }
    if (j < flybyes.size()) {
      ss << flybyes.Get(j++);
    }
  }

  ss << sol.arrival();

  ss << "Total delta-V:           " << sol.fuel_cost() << std::setprecision(3) << " m/s" << std::endl;

  return ss;
}



std::ostream& __cdecl operator<<(std::ostream& ss, const TransXTimes &tim) {
  ss << std::fixed;

  std::vector<std::string> encounters;
  std::vector<std::pair<std::string, std::string> > transfers;
  for (int i = 0; i < tim.planets().size() - 1; ++i) {
    ss << "Transfer time from " << tim.planets().Get(i) << " to " << tim.planets().Get(i+1) << ":";
    ss << (tim.times().Get(i + 1) - tim.times().Get(i)) << std::setprecision(2) << " days" << std::endl;
  }

  for (int i = 0; i < tim.planets().size(); ++i) {
    ss << "Date of " << tim.planets().Get(i) << " encounter: ";
    ss << tim.times().Get(i) << std::endl;
  }

  ss << "Total mission duration: " << (tim.times().Get(tim.times().size() - 1) - tim.times().Get(0)) << std::setprecision(2) << " days" << std::endl << std::endl << std::endl;

  return ss;
}

std::ostream& __cdecl operator<<(std::ostream& ss, const TransXEscape &esc) {
  ss << std::fixed;
  ss << "Escape - " << esc.planet() << std::endl;
  ss << "--------------------------------------" << std::endl;
  ss << "MJD:                     " << esc.mjd() << std::setprecision(4) << std::endl;
  ss << "Prograde:                " << esc.prograde() << std::setprecision(3) << std::endl;
  ss << "Outward:                 " << esc.outward() << std::setprecision(3) << std::endl;
  ss << "Plane:                   " << esc.plane() << std::setprecision(3) << std::endl;
  ss << "Hyp. excess velocity:    " << esc.vinf() << std::setprecision(3) <<  " m/s" << std::endl;
  ss << "Earth escape burn:       " << esc.burn() << std::setprecision(3) << std::endl;
  ss << std::endl << std::endl;

  return ss;
};

std::ostream& __cdecl operator<<(std::ostream& ss, const TransXDSM &dsm) {

  ss << std::fixed;
  ss << "Deep Space Maneuver - " << std::endl;
  ss << "--------------------------------------" << std::endl;
  ss << "MJD:                     " << dsm.mjd() << std::setprecision(4) << std::endl;
  ss << "Prograde:                " << dsm.prograde() << std::setprecision(3) << std::endl;
  ss << "Outward:                 " << dsm.outward() << std::setprecision(3) << std::endl;
  ss << "Plane:                   " << dsm.plane() << std::setprecision(3) << std::endl;
  ss << "Hyp. excess velocity:    " << dsm.vinf() << std::setprecision(3) <<  "m/s" << std::endl;
  ss << "DSM burn:                " << dsm.burn() << std::setprecision(3) << std::endl;

  ss << std::endl << std::endl;

  return ss;
}

std::ostream& __cdecl operator<<(std::ostream& ss, const TransXFlyBy &f) {
  ss << std::fixed;
  ss << f.planet() << " encounter" << std::endl;
  ss << "--------------------------------------" << std::endl;
  ss << "MJD:                   " << f.mjd() << std::setprecision(4) << std::endl;
  ss << "Approach velocity:     " << f.approach_vel() << std::setprecision(3) << std::endl;
  ss << "Departure velocity:    " << f.departure_vel() << std::setprecision(3) << std::endl;

  ss << "Outward angle:         " << f.outward_angle() << std::endl;
  ss << "Inclination:           " << f.inclination() << std::setprecision(3) << " deg" << std::endl;
  ss << "Turning angle:         " << f.turning_angle() << std::setprecision(3) << " deg" << std::endl;
  ss << "Periapsis altitude:    " << f.periapsis_altitude() << std::setprecision(3) << " km" << std::endl;
  ss << "dV needed:             " << f.burn() << std::setprecision(3) << " m/s" << std::endl;
  ss << std::endl << std::endl;

  return ss;
}

}


