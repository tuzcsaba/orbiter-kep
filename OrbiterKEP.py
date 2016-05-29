from PyGMO import archipelago, problem, island
from PyGMO.algorithm import jde,mbh,cs
from PyGMO.topology import ring, fully_connected
from PyKEP import epoch, epoch_from_iso_string
from PyKEP.planet import jpl_lp,spice
from PyKEP.trajopt import mga_1dsm
from PyKEP.util import load_spice_kernel
from problems.my_mga_1dsm import my_mga_1dsm
from problems.my_mga import my_mga
from scipy.linalg import norm
import logging
import sys
import os

from optimise import optimizer

for root, dirs, filenames in os.walk('spice'):
  for f in filenames:
    load_spice_kernel('./spice/' + f)

argv = sys.argv

if len(argv) < 6:
  print("usage: " + argv[0] + " <comma delimited planets> <t0_min,t0_max> <T_min, T_max> <max vinf> <n_mga> <n_mga_1dsm>")
  exit(0)

planets = argv[1].split(',')
launch = [epoch_from_iso_string(x) for x in argv[2].split(',')]
tof = [float(x) for x in argv[3].split(',')]

# We define an Earth-Venus-Earth problem (single-objective)
seq = [jpl_lp(name) for name in argv[1].split(',')]
# seq = [spice(name) for name in argv[1].split(',')]
# seq = [jpl_lp('earth'), jpl_lp('venus'), jpl_lp('venus'), jpl_lp('earth'), jpl_lp('jupiter'), jpl_lp('saturn')]

# enc_times = [50736.36319, 50929.5, 51353.5, 51408.14444, 51908.5, 53187.11667]
enc_times = []

runMGA = True
runMGA_1DSM = True 

sol_mga = None
t = None

nTrialMGA = int(argv[5])
nTrialMGA1DSM = int(argv[6])

sol_mga_1dsm = None
try:
  if runMGA:
    mga = my_mga(seq=seq, t0 = launch, tof = tof, add_vinf_dep = True, add_vinf_arr = True, multi_objective = False)
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
    vinf = [0.1, float(argv[4])]
    # if sol_mga != None:
    #   t0 = sol_mga[0]
    #   T = sol_mga[1] / 365.25
    #   launch = [epoch(t0 - 30), epoch(t0 + 30)]
    #   tof = [0.1, T * 1.25]
    mga_1dsm = my_mga_1dsm(seq=seq, t0=launch, tof = tof, vinf=vinf, mga_sol = None, add_vinf_dep = True, add_vinf_arr = True, multi_objective = True)
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

