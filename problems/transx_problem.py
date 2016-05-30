from PyGMO.problem import base as base_problem
from PyKEP.core import epoch, DAY2SEC, MU_SUN, lambert_problem, propagate_lagrangian, fb_prop, fb_vel, AU
from PyKEP.planet import jpl_lp,spice
from math import sqrt, pi, cos, sin, acos, atan2, log
from scipy.linalg import norm
from numpy import *

excludes = []

class transx_problem(base_problem):
    """
    This class is a PyGMO (http://esa.github.io/pygmo/) problem representing a Multiple Gravity Assist
    trajectory allowing one only impulsive Deep Space Manouvre between each leg.

    - Izzo, Dario. "Global optimization and space pruning for spacecraft trajectory design." Spacecraft Trajectory Optimization 1 (2010): 178-200.

    The decision vector is::

      [t0, T] + [u, v, Vinf, eta1, a1] + [beta, rp/rV, eta2, a2] + ...

    ... in the units: [mjd2000, days] + [nd,nd,km/s,nd,years,nd] + [rad,nd,nd,nd] + ....

    Each leg time-of-flight can be decoded as follows, T_n = T log(alpha_n) / \sum_i(log(alpha_i)).

    .. note::

       The resulting problem is box-bounded (unconstrained). The resulting trajectory is time-bounded.
    """

    def exclude(self, x):
        excludes.append(x)

    def __init__(self, seq=[jpl_lp('earth'), jpl_lp('venus'), jpl_lp('earth')], dim = 1, obj_dim  = 1, avoid = []):

        # Sanity checks ...... all planets need to have the same
        # mu_central_body
        if ([r.mu_central_body for r in seq].count(seq[0].mu_central_body) != len(seq)):
            raise ValueError('All planets in the sequence need to have exactly the same mu_central_body')
        self.n_legs = len(seq) - 1
        
        # We then define all planets in the sequence  and the common central
        # body gravity as data members
        self.seq = seq
        self.common_mu = seq[0].mu_central_body
        if self.common_mu == 0:
          self.common_mu = 1e10
        self.avoid = []
        # First we call the constructor for the base PyGMO problem
        # As our problem is n dimensional, box-bounded (may be multi-objective), we write
        # (dim, integer dim, number of obj, number of con, number of inequality con, tolerance on con violation)
        super(transx_problem, self).__init__(dim, 0, obj_dim, 0, 0, 0)


    def print_time_info(self, planets, times):
        # print out the results

        encounters = [] 
        transfers = []
        for i in range(len(planets) - 1):
          src = planets[i]
          dst = planets[i + 1]
          transfers.append(['Transfer time from ' + src.name + ' to ' + dst.name + ':', str(round(times[i + 1].mjd - times[i].mjd, 2)), " days"])

        for i in range(len(planets)):
          x = planets[i]
          encounters.append("Date of " + x.name + ' encounter: ' + str(times[i]))

        for enc in encounters:
          print(enc)


        max_l = max(map(lambda x: len(x[0]), transfers))

        transfers = map(lambda x: x[0].ljust(max_l) + x[1] + x[2], transfers)
        for tran in transfers:
          print(tran)
            
        print "Total mission duration:             ", round(times[-1].mjd - times[0].mjd, 2), " days"
        print
        print
       
        
    def print_escape(self, ref, V_ref, R_ref, deltaV, eject_T):
        dVTransX = self.velocity_to_transx(V_ref, R_ref, deltaV)
        dv = norm(deltaV)

        print "TransX escape plan - " + ref.name + " escape"
        print "--------------------------------------"
        print("MJD:                 %10.4f "    % round(eject_T, 4))
        print("Prograde:            %10.3f m/s" % round(dVTransX[0], 3))
        print("Outward:             %10.3f m/s" % round(dVTransX[1], 3))
        print("Plane:               %10.3f m/s" % round(dVTransX[2], 3))
        print("Hyp. excess velocity:%10.3f m/s" % round(dv, 3))
        print("Earth escape burn:   %10.3f m/s" % round(self.burn_cost(ref, deltaV), 3))
        print
        print 

    def print_dsm(self, V_ref, R_ref, deltaV, v, dsm_T):
        dVTransX = self.velocity_to_transx(V_ref, R_ref, deltaV)
        dv = norm(deltaV)

        print "Deep Space Burn"
        print "--------------------------------------"
        print("MJD:                 %10.4f "    % round(dsm_T, 4))
        print("Prograde:            %10.3f m/s" % round(dVTransX[0], 3))
        print("Outward:             %10.3f m/s" % round(dVTransX[1], 3))
        print("Plane:               %10.3f m/s" % round(dVTransX[2], 3))
        print("Hyp. excess velocity:%10.3f m/s" % round(norm(v), 3))
        print("DSM burn:            %10.3f m/s" % round(dv, 3))
        print
        print 
        

    def burn_cost(self, ref, exc, circ = True):
        mu = ref.mu_self
        r = ref.radius + 200000 
        dv = []
        return sqrt(dot(exc, exc) + 2 * mu / r) - sqrt((1 if circ else 2) * mu / r)
        
    def print_flyby(self, ref, V_ref, R_ref, Vin, Vout, enc_T):
        VoutTransX = self.velocity_to_transx(V_ref, R_ref, Vout)

        vx = VoutTransX[0]
        vy = VoutTransX[1]
        vz = VoutTransX[2]

        print ref.name + " encounter"
        print "--------------------------------------"
        print("MJD:                 %10.4f "    % round(enc_T, 4))
        print("Approach velocity:   %10.3f m/s" % round(sqrt(dot(Vin,Vin)), 3))
        print("Departure velocity:  %10.3f m/s" % round(sqrt(dot(Vout,Vout)), 3))  
        print("Outward angle:       %10.3f deg" % round(180*atan2(vy, vx)/pi, 3))
        print("Inclination:         %10.3f deg" % round(180*atan2(vz, sqrt(vx * vx + vy * vy))/pi, 3))
        ta  = acos(dot(Vin, Vout)/sqrt(dot(Vin,Vin))/sqrt(dot(Vout,Vout)))
        print("Turning angle:       %10.3f deg" % round(ta*180/pi, 3))
        alt = (ref.mu_self / dot(Vin,Vin)*(1/sin(ta/2)-1) - ref.radius)/1000
        print("Periapsis altitude:  %10.3f km " % round(alt, 3))
        print("dV needed:           %10.3f m/s" % round(fb_vel(Vin, Vout, ref), 3))
        print
        print 
 
    def print_arrival(self, ref, V_exc, enc_T):

        print ref.name + " arrival"
        print "--------------------------------------"
        print("MJD:                 %10.4f    " % round(enc_T, 4))
        print("Hyp. excess velocity:%10.3f m/s" % round(norm(V_exc), 3))
        print("Orbit insertion burn %10.3f m/s" % round(self.burn_cost(ref, V_exc), 3))
        print
        print

    def velocity_to_transx(self, v_ref, v_rad, v):
        fward = v_ref / linalg.norm(v_ref)
        plane = cross(v_ref, v_rad); plane = plane / linalg.norm(plane)
        oward = cross(plane, fward)

        vx = dot(fward, v)
        vy = dot(oward, v)
        vz = dot(plane, v)

        return [vx, vy, vz]

    def calc_objective(self, x, should_print = False):
   
        if self.f_dimension == 1:
            return (0,)
        else:
            return (0, 0)

    # Objective function
    def _objfun_impl(self, x):
        if len(self.avoid) > 0:
          d = max([(exp(norm(x) / (norm([a - b for a, b in zip(x, y)]))) - 1) for y in self.avoid])
          return d + self.calc_objective(x)


        return self.calc_objective(x)

    def pretty(self, x):
        # Plot of the trajectory
        self.calc_objective(x, True)

