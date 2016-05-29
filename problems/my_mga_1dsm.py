from PyGMO.problem import base as base_problem
from PyKEP.core import epoch, DAY2SEC, MU_SUN, lambert_problem, propagate_lagrangian, fb_prop, fb_vel, AU
from PyKEP.planet import jpl_lp
from math import sqrt, pi, cos, sin, acos, atan2, log
from scipy.linalg import norm
from numpy import *
from my_prob import my_prob

excludes = []

class my_mga_1dsm(my_prob):
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

    def __init__(self, seq=[jpl_lp('earth'), jpl_lp('venus'), jpl_lp('earth')], t0=[epoch(0), epoch(1000)], tof=[1.0, 5.0], vinf=[0.5, 2.5], mga_sol = None,  add_vinf_dep=False, add_vinf_arr=True, multi_objective=False, avoid = []):
        """
        PyKEP.trajopt.mga_1dsm(seq = [jpl_lp('earth'), jpl_lp('venus'), jpl_lp('earth')], t0 = [epoch(0),epoch(1000)], tof = [1.0,5.0], vinf = [0.5, 2.5], multi_objective = False, add_vinf_dep = False, add_vinf_arr=True)

        - seq: list of PyKEP planets defining the encounter sequence (including the starting launch)
        - t0: list of two epochs defining the launch window
        - tof: list of two floats defining the minimum and maximum allowed mission lenght (years)
        - vinf: list of two floats defining the minimum and maximum allowed initial hyperbolic velocity (at launch), in km/sec
        - multi_objective: when True constructs a multiobjective problem (dv, T)
        - add_vinf_dep: when True the computed Dv includes the initial hyperbolic velocity (at launch)
        - add_vinf_arr: when True the computed Dv includes the final hyperbolic velocity (at the last planet)
        """

        # First we call the constructor for the base PyGMO problem
        # As our problem is n dimensional, box-bounded (may be multi-objective), we write
        # (dim, integer dim, number of obj, number of con, number of inequality con, tolerance on con violation)
        dim = 7 + (len(seq) - 2) * 4
        obj_dim = multi_objective + 1
        super(my_mga_1dsm, self).__init__(seq, dim, obj_dim, avoid)

        self.__add_vinf_dep = add_vinf_dep
        self.__add_vinf_arr = add_vinf_arr

        # And we compute the bounds
        lb = [t0[0].mjd2000, tof[0] * 365.25] + [0.0, 0.0, vinf[0] * 1000, 1e-5, 1e-5] + [-2 * pi, 1.1, 1e-5, 1e-5] * (self.n_legs - 1)
        ub = [t0[1].mjd2000, tof[1] * 365.25] + [1.0, 1.0, vinf[1] * 1000, 1.0 - 1e-5, 1.0 - 1e-5] + [2 * pi, 30.0, 1.0 - 1e-5, 1.0 - 1e-5] * (self.n_legs - 1)

        self.__mga_sol = mga_sol

        # Accounting that each planet has a different safe radius......
        for i, pl in enumerate(seq[1:-1]):
            lb[8 + 4 * i] = pl.safe_radius / pl.radius

        # And we set them
        self.set_bounds(lb, ub)

    def _decode_times_and_vinf(self, x):
        T = list([0] * (self.n_legs))
        for i in range(len(T)):
            if self.__mga_sol != None:
              d = self.__mga_sol[i + 1].mjd - self.__mga_sol[i].mjd

              T[i] = d + 0 * (i + 1) * (x[6 + 4 * i] - 0.5)
            else:
              T[i] = -log(x[6 + 4 * i])
        alpha_sum = sum(T)

        theta = 2 * pi * x[2]
        phi = acos(2 * x[3] - 1) - pi / 2

        Vinfx = x[4] * cos(phi) * cos(theta)
        Vinfy = x[4] * cos(phi) * sin(theta)
        Vinfz = x[4] * sin(phi)

        # E = Desired encounted
        # kell: T * p = E - x[0]
        # p = E / T
        # (E + x * 0.1) / T 

        return ([x[1] * time / alpha_sum for time in T], Vinfx, Vinfy, Vinfz)
    
    def calc_objective(self, x, should_print = False):
        # 1 -  we 'decode' the chromosome recording the various times of flight
        # (days) in the list T and the cartesian components of vinf
        T, Vinfx, Vinfy, Vinfz = self._decode_times_and_vinf(x)

        Vinf = [Vinfx, Vinfy, Vinfz]

        # 2 - We compute the epochs and ephemerides of the planetary encounters
        t_P = list([None] * (self.n_legs + 1))
        r_P = list([None] * (self.n_legs + 1))
        v_P = list([None] * (self.n_legs + 1))
        DV = list([0.0] * (self.n_legs + 1))
        for i, planet in enumerate(self.seq):
            t_P[i] = epoch(x[0] + sum(T[0:i]))
            r_P[i], v_P[i] = self.seq[i].eph(t_P[i])

        if should_print:
          self.print_time_info(self.seq, t_P)

        if self.__add_vinf_dep:
            DV[0] += self.burn_cost(self.seq[0], Vinf)
            if should_print:
              self.print_escape(self.seq[0], v_P[0], r_P[0], Vinf, t_P[0].mjd)

        # 3 - We start with the first leg
        v0 = [a + b for a, b in zip(v_P[0], Vinf)]
        r, v = propagate_lagrangian(r_P[0], v0, x[5] * T[0] * DAY2SEC, self.common_mu)

        # Lambert arc to reach seq[1]
        dt = (1 - x[5]) * T[0] * DAY2SEC
        l = lambert_problem(r, r_P[1], dt, self.common_mu, False, False)
        v_beg_l = l.get_v1()[0]
        v_end_l = l.get_v2()[0]

        # First DSM occuring at time nu1*T1
        deltaV = [a - b for a, b in zip(v_beg_l, v)]
        DV[0] += norm(deltaV) 

        if should_print:
          self.print_dsm(v, r, deltaV, v_beg_l, t_P[0].mjd + dt / DAY2SEC)


        # 4 - And we proceed with each successive leg
        for i in range(1, self.n_legs):
            # Fly-by
            radius = x[8 + (i - 1) * 4] * self.seq[i].radius
            beta = x[7 + (i - 1) * 4]
            v_out = fb_prop(v_end_l, v_P[i],radius , beta, self.seq[i].mu_self)

            if should_print:
                v_rel_in = [a - b for a,b in zip(v_end_l, v_P[i])]
                v_rel_out = [a - b for a,b in zip(v_out, v_P[i])]
                self.print_flyby(self.seq[i], v_P[i], r_P[i], v_rel_in, v_rel_out, t_P[i].mjd)

            # s/c propagation before the DSM
            r, v = propagate_lagrangian(r_P[i], v_out, x[9 + (i - 1) * 4] * T[i] * DAY2SEC, self.common_mu)
            # Lambert arc to reach Earth during (1-nu2)*T2 (second segment)
            dt = (1 - x[9 + (i - 1) * 4]) * T[i] * DAY2SEC
            l = lambert_problem(r, r_P[i + 1], dt, self.common_mu, False, False)
            v_end_l = l.get_v2()[0]
            v_beg_l = l.get_v1()[0]
            # DSM occuring at time nu2*T2
            deltaV = [a - b for a, b in zip(v_beg_l, v)]
            DV[i] = norm(deltaV)
            if should_print:
              self.print_dsm(v, r, deltaV, v_beg_l, t_P[i].mjd + dt / DAY2SEC)

        # Last Delta-v
        if self.__add_vinf_arr:
            Vexc_arr = [a - b for a, b in zip(v_end_l, v_P[-1])]
            DV[-1] = self.burn_cost(self.seq[-1], Vexc_arr)
            if should_print:
                self.print_arrival(self.seq[-1], Vexc_arr, t_P[-1].mjd)

        fuelCost = sum(DV)

        if should_print:
          print("Total fuel cost:     %10.3f m/s" % round(fuelCost, 3))
   
        if self.f_dimension == 1:
            return (fuelCost,)
        else:
            return (fuelCost, sum(T))


    # Objective function
    def _objfun_impl(self, x):
        if len(excludes) == 0:
            return self.calc_objective(x)

        nearest_excl = min(excludes, key = lambda y: norm([abs(a - b) for a, b in zip(x, y)]))
        d = norm([abs(a - b) for a, b in zip(nearest_excl, x)])
        return exp(1 / (norm(x) * d)) + self.calc_objective(x)

    def pretty(self, x):
        # Plot of the trajectory
        self.calc_objective(x, True)

    def plot(self, x, ax=None):
        """
        ax = prob.plot(x, ax=None)

        - x: encoded trajectory
        - ax: matplotlib axis where to plot. If None figure and axis will be created
        - [out] ax: matplotlib axis where to plot

        Plots the trajectory represented by a decision vector x on the 3d axis ax

        Example::

          ax = prob.plot(x)
        """
        import matplotlib as mpl
        from mpl_toolkits.mplot3d import Axes3D
        import matplotlib.pyplot as plt
        from PyKEP.orbit_plots import plot_planet, plot_lambert, plot_kepler

        if ax is None:
            mpl.rcParams['legend.fontsize'] = 10
            fig = plt.figure()
            axis = fig.gca(projection='3d')
        else:
            axis = ax

        axis.scatter(0, 0, 0, color='y')

        # 1 -  we 'decode' the chromosome recording the various times of flight
        # (days) in the list T and the cartesian components of vinf
        T, Vinfx, Vinfy, Vinfz = self._decode_times_and_vinf(x)

        # 2 - We compute the epochs and ephemerides of the planetary encounters
        t_P = list([None] * (self.n_legs + 1))
        r_P = list([None] * (self.n_legs + 1))
        v_P = list([None] * (self.n_legs + 1))
        DV = list([None] * (self.n_legs + 1))

        for i, planet in enumerate(self.seq):
            t_P[i] = epoch(x[0] + sum(T[0:i]))
            r_P[i], v_P[i] = planet.eph(t_P[i])
            plot_planet(planet, t0=t_P[i], color=(0.8, 0.6, 0.8), legend=True, units = AU, ax=axis)

        # 3 - We start with the first leg
        v0 = [a + b for a, b in zip(v_P[0], [Vinfx, Vinfy, Vinfz])]
        r, v = propagate_lagrangian(r_P[0], v0, x[5] * T[0] * DAY2SEC, self.common_mu)

        plot_kepler(r_P[0], v0, x[5] * T[0] * DAY2SEC, self.common_mu, N=100, color='b', legend=False, units=AU, ax=axis)

        # Lambert arc to reach seq[1]
        dt = (1 - x[5]) * T[0] * DAY2SEC
        l = lambert_problem(r, r_P[1], dt, self.common_mu, False, False)
        plot_lambert(l, sol=0, color='r', legend=False, units=AU, ax=axis)
        v_end_l = l.get_v2()[0]
        v_beg_l = l.get_v1()[0]

        # First DSM occuring at time nu1*T1
        DV[0] = norm([a - b for a, b in zip(v_beg_l, v)])

        # 4 - And we proceed with each successive leg
        for i in range(1, self.n_legs):
            # Fly-by
            v_out = fb_prop(v_end_l, v_P[i], x[8 + (i - 1) * 4] * self.seq[i].radius, x[7 + (i - 1) * 4], self.seq[i].mu_self)
            # s/c propagation before the DSM
            r, v = propagate_lagrangian(r_P[i], v_out, x[9 + (i - 1) * 4] * T[i] * DAY2SEC, self.common_mu)
            plot_kepler(r_P[i], v_out, x[9 + (i - 1) * 4] * T[i] * DAY2SEC, self.common_mu, N=100, color='b', legend=False, units=AU, ax=axis)
            # Lambert arc to reach Earth during (1-nu2)*T2 (second segment)
            dt = (1 - x[9 + (i - 1) * 4]) * T[i] * DAY2SEC

            l = lambert_problem(r, r_P[i + 1], dt, self.common_mu, False, False)
            plot_lambert(l, sol=0, color='r', legend=False, units=AU, N=1000, ax=axis)

            v_end_l = l.get_v2()[0]
            v_beg_l = l.get_v1()[0]
            # DSM occuring at time nu2*T2
            DV[i] = norm([a - b for a, b in zip(v_beg_l, v)])
        plt.show()
        return axis

    def set_tof(self, minimum, maximum):
        """
        prob.set_tof(minimum, maximum)

        - minimum: minimum tof (in years)
        - maximum: maximum tof (in years)

        Sets the minimum and maximum time of flight allowed (in years)

        Example::

          m = 3
          M = 5
          prob.set_tof(m,M)
        """
        lb = list(self.lb)
        ub = list(self.ub)
        lb[1] = minimum * 365.25
        ub[1] = maximum * 365.25
        self.set_bounds(lb, ub)

    def set_launch_window(self, start, end):
        """
        prob.set_launch_window(start, end)

        - start: starting epoch
        - end: ending epoch

        Sets the launch window allowed in terms of starting and ending epochs

        Example::

          start = epoch(0)
          end = epoch(1000)
          prob.set_launch_window(start, end)
        """
        lb = list(self.lb)
        ub = list(self.ub)
        lb[0] = start.mjd2000
        ub[0] = end.mjd2000
        self.set_bounds(lb, ub)

    def set_vinf(self, vinf):
        """
        prob.set_vinf(vinf)

        - vinf: allowed launch vinf (in km/s)

        Sets the allowed launch vinf (in km/s)

        Example::

          M = 5
          prob.set_vinf(M)
        """
        lb = list(self.lb)
        ub = list(self.ub)
        lb[4] = 0
        ub[4] = vinf * 1000
        self.set_bounds(lb, ub)

    def human_readable_extra(self):
        return ("\n\t Sequence: " + [pl.name for pl in self.seq].__repr__() +
                "\n\t Add launcher vinf to the objective?: " + self.__add_vinf_dep.__repr__() +
                "\n\t Add final vinf to the objective?: " + self.__add_vinf_arr.__repr__())
