from PyGMO import archipelago, problem, island
from PyGMO.algorithm import jde,mbh,cs
from PyGMO.topology import ring, fully_connected
from PyKEP import epoch, epoch_from_iso_string
from PyKEP.planet import jpl_lp,spice
from PyKEP.trajopt import mga_1dsm
from PyKEP.util import load_spice_kernel
from problems.mga_1dsm_transx import mga_1dsm_transx
from problems.mga_transx import mga_transx 
from scipy.linalg import norm
import argparse
import logging
import sys
import os

from optimise import optimizer

parser = argparse.ArgumentParser(description = "Calculate Optimal MGA and MGA-1DSM trajectories")
parser.add_argument('--planets', default='earth,venus,mercury', help='encountered planetary bodies')
parser.add_argument('--launch', default='20000101T000000,20140101T000000', help='the bounds for the launch')
parser.add_argument('--tof', default='0.5,5', help='the bounds for the time of flight')
parser.add_argument('--vinf-max', dest='vinf_max', default='10', help='maximum allowed initial Hyperbolic Excess Velocity (check Launcher performance)')
parser.add_argument('--n-mga', dest='n_mga', default='1', help='the number of independent MGA optimisations')
parser.add_argument('--n-mga-1dsm', dest='n_mga_1dsm', default='0', help='the number of independent MGA-1DSM optimisations')
parser.add_argument('--multi-obj', dest='multi_obj', action='store_const', const=True, default=False, help='If true, then we optimise for DeltaV and Time Of Flight.')

args = parser.parse_args()

for root, dirs, filenames in os.walk('spice'):
  for f in filenames:
    load_spice_kernel('./spice/' + f)

launch = [epoch_from_iso_string(x) for x in args.launch.split(',')]
tof = [float(x) for x in args.tof.split(',')]
vinf = [1e-2, float(args.vinf_max)]

# We define an Earth-Venus-Earth problem (single-objective)
seq = [jpl_lp(name) for name in args.planets.split(',')]
# seq = [spice(name) for name in args.planets.split(',')]
# seq = [jpl_lp('earth'), jpl_lp('venus'), jpl_lp('venus'), jpl_lp('earth'), jpl_lp('jupiter'), jpl_lp('saturn')]

# enc_times = [50736.36319, 50929.5, 51353.5, 51408.14444, 51908.5, 53187.11667]
enc_times = []

runMGA = True
runMGA_1DSM = True 

sol_mga = None
t = None

nTrialMGA = int(args.n_mga)
nTrialMGA1DSM = int(args.n_mga_1dsm)

multi_obj = args.multi_obj

sol_mga_1dsm = None
try:
  if runMGA:
    mga = mga_transx(seq=seq, t0 = launch, tof = tof, add_vinf_dep = True, add_vinf_arr = True, multi_objective = multi_obj)
    solutions = []
    for i in range(nTrialMGA):
      # mga.avoid = solutions
      op = optimizer(mga, 100)
      sol_mga = op.run_once()
      if sol_mga != None:
        solutions.append(sol_mga)

    if (len(solutions) > 0):
      sol_mga = min(solutions, key=lambda x: mga.calc_objective(x))

  if runMGA_1DSM:
    solutions = []
    # if sol_mga != None:
    #   t0 = sol_mga[0]
    #   T = sol_mga[1] / 365.25
    #   launch = [epoch(t0 - 30), epoch(t0 + 30)]
    #   tof = [0.1, T * 1.25]
    mga_1dsm = mga_1dsm_transx(seq=seq, t0=launch, tof = tof, vinf=vinf, mga_sol = None, add_vinf_dep = True, add_vinf_arr = True, multi_objective = multi_obj)
    for i in range(nTrialMGA1DSM):

      # mga_1dsm.avoid = solutions
      op = optimizer(mga_1dsm, 100)
      sol_mga_1dsm = op.run_once()
      if sol_mga_1dsm != None:
        solutions.append(sol_mga_1dsm)

    if (len(solutions) > 0):
      sol_mga_1dsm = min(solutions, key=lambda x: mga_1dsm.calc_objective(x))
except Exception, e:
  logging.error(e, exc_info = True)
finally:
  if sol_mga != None:
    mga.pretty(sol_mga)
  if sol_mga_1dsm != None:
    mga_1dsm.pretty(sol_mga_1dsm)

