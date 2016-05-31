from PyGMO import problem
from PyKEP import *
from PyKEP.planet import * 
from transx_problem import transx_problem

from math import *
from numpy import *

class mga_transx(transx_problem):
    """
    MGA problem:

    Decision vector:
    [t0, a1, a2, ...]

    This class is a PyGMO (http://esa.github.io/pygmo/) problem representing a Mutltiple Gravity Assist
    trajectory, with possibility engine firing only at the beginning, and end of each legs.

    - Izzo, Dario. "Global optimization and space pruning for spacecraft trajectory design." Spacecraft Trajectory Optimization 1 (2010): 178-200.

    The decision vector is:

      [t0] + [a1, a2, a3, a4, ...]

    ... in the units: [mjd2000] + [nd, nd, nd, nd, ...]

    Each leg time of flight can be decoded as follows, T_n = T * log(alpha_n) / \sum_i(log(alpha_i)) 

    .. note::

      The resulting problem is box-bounded (unconstrained). The resulting trajectory is time-bounded


    """

    def __init__(self, seq = [jpl_lp('earth'), jpl_lp('venus'), jpl_lp('mercury')], t0 = [epoch(0), epoch(1000)], tof=[1.0, 5.0], add_vinf_dep = False, add_vinf_arr = True, multi_objective=False, avoid = []):
        dim = len(seq)
        obj_dim = multi_objective + 1
        super(mga_transx, self).__init__(seq, dim, obj_dim, avoid)  # a problem in three variables

        self.__add_vinf_dep = add_vinf_dep
        self.__add_vinf_arr = add_vinf_arr

        lb = [t0[0].mjd2000, tof[0] * 365.25] + [1e-5] * (self.n_legs - 1)
        ub = [t0[1].mjd2000, tof[1] * 365.25] + [1.0 - 1e-5] * (self.n_legs - 1)

        self.set_bounds(lb, ub)

    def calc_objective(self, x,  should_print = False):
        # calculate the state vectors of Earth, Venus & Mercury using JPL's Low Precision
        # ephemeris

        T = self._decode_times(x)
        
        t_P = list([None] * (len(self.seq)))
        r_P = list([None] * (len(self.seq)))
        v_P = list([None] * (len(self.seq)))
        DV = list([0.0] * (len(self.seq)))
        for i,planet in enumerate(self.seq):
          t_P[i] = epoch(x[0] + sum(T[0:i]))
          r_P[i], v_P[i] = planet.eph(t_P[i])

        if should_print:
          self.print_time_info(self.seq, t_P)

        vout = 0
        vin = 0

        for i in range(0, self.n_legs):
          r, v = r_P[i], v_P[i]

          dt = (t_P[i + 1].mjd - t_P[i].mjd) * DAY2SEC
          l = lambert_problem(r, r_P[i + 1], dt, self.common_mu, False, False)
          v_beg_l = l.get_v1()[0]
          v_end_l = l.get_v2()[0]

          vout = v_beg_l

          if i == 0:
            vout_rel = [a - b for a, b in zip(vout, v_P[i])]
            if self.__add_vinf_dep:
              DV[0] = self.burn_cost(self.seq[0], vout_rel)
            if should_print:
              self.print_escape(self.seq[0], v_P[0], r_P[0], vout_rel, t_P[0].mjd)
          else:
            v_rel_in = [a - b for a, b in zip(vin, v_P[i])]
            v_rel_out = [a - b for a, b in zip(vout, v_P[i])]
            DV[i] = fb_vel(v_rel_in, v_rel_out, self.seq[i])
            if should_print:
              self.print_flyby(self.seq[i], v_P[i], r_P[i], v_rel_in, v_rel_out, t_P[i].mjd)

          vin = v_end_l

        if self.__add_vinf_arr:
          Vexc_arr = [a - b for a, b in zip(v_end_l, v_P[-1])]
          DV[-1] = self.burn_cost(self.seq[-1], Vexc_arr)
          if should_print:
            self.print_arrival(self.seq[-1], Vexc_arr, t_P[-1].mjd)

        fuelCost = sum(DV)
        cost = fuelCost
        cost = cost if self.__add_vinf_dep else cost + self.burn_cost(self.seq[0], vout_rel)

        if (should_print):
          print("Total fuel cost:     %10.3f m/s" % round(cost, 3))

        if self.f_dimension == 1:
          return (fuelCost,)
        else:
          return (fuelCost, sum(T))
      
    def _decode_times(self, x):
      T = list([0.0] * self.n_legs)
      t = []
      u = 0
      n = len(x) - 2
      t.append(0.0)
      for i in range(n, 0, -1):
        val = x[2 + i - 1]
        u += (1 - u) * (1 - pow(val, 1. / i))
        t.append(u)
      t.append(1.0)
      for i in range(0, len(T)):
        T[i] = t[i + 1] - t[i]
      alpha_sum = sum(T)

      res = [x[1] * time / alpha_sum for time in T]
      return res

    def plot(self, x):
        # Making sure the leg corresponds to the requested chromosome
        # self._compute_constraints_impl(x)

        # Plotting commands
        fig = plt.figure()
        axis = fig.gca(projection='3d')
        # The Sun
        axis.scatter([0], [0], [0], color='y')
        # The leg
        for leg in self.__legs:
            orbit_plots.plot_lambert(leg.solution, units=AU, N=100, ax=axis)

        # The planets
        for p in self.seq:
            orbit_plots.plot_planet(plnt = p.planet, t0 = p.t, units=AU, legend=True, color=(0.8, 0.8, 1.0), ax = axis)
        plt.show()

    # Add some output to __repr__
    def human_readable_extra(self):
      planetStr = reduce(lambda x,y: x + '-' + y, map(lambda x: x.planet.name, self.__planets))
      return "\n\tMinimisation problem - " + planetStr + " transfer"



