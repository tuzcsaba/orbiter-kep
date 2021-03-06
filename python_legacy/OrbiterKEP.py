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
import matplotlib.pylab as plt
import argparse
import logging
import sys
import os

from optimise import optimizer

parser.add_argument('--ship-max-deltaV', dest='max_deltav', help='your ship\'s Delta-V budget', default = 20000)

args = parser.parse_args()

for root, dirs, filenames in os.walk('spice'):
  for f in filenames:
    load_spice_kernel('./spice/' + f)

launch = [epoch_from_iso_string(x) for x in args.launch.split(',')]
tof = [float(x) for x in args.tof.split(',')]
vinf = [0.5, float(args.vinf_max)]

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
    mga = mga_transx(seq=seq, 
                     dep_altitude = args.dep_altitude, 
                     arr_altitude = args.arr_altitude, 
                     circularize = args.circularize,
                     t0 = launch, tof = tof, 
                     add_vinf_dep = args.add_dep_vinf, add_vinf_arr = args.add_arr_vinf,
                     multi_objective = False)
    mga_multi = None
    if multi_obj:
      mga_multi = mga_transx(seq=seq, 
                             dep_altitude = args.dep_altitude, 
                             arr_altitude = args.arr_altitude, 
                             circularize = args.circularize,
                             t0 = launch, tof = tof, 
                             add_vinf_dep = args.add_dep_vinf, add_vinf_arr = args.add_arr_vinf,
                             multi_objective = multi_obj)
    solutions = []
    for i in range(nTrialMGA):
      print("MGA problem - Iteration %d" % (i + 1))
      # mga.avoid = solutions
      op = optimizer(mga, nTrial = 5, gen = 1000, mf = 100, mr = 1)
      sol_mga = op.run_once()
      if multi_obj:
        print("MGA problem - Multi-Objective phase - Iteration %d" % (i + 1))
        op = optimizer(mga_multi, 1, gen = 4000, mf = 100, mr = 10)
        sol_mga_multi = op.run_once(sol_mga, True)

      if sol_mga != None:
        solutions.append(sol_mga)

    if (len(solutions) > 0):
      sol_mga = min(solutions, key=lambda x: mga.calc_objective(x))

  if runMGA_1DSM:
    mga_1dsm = mga_1dsm_transx(seq=seq, 
                               dep_altitude = args.dep_altitude, 
                               arr_altitude = args.arr_altitude, 
                               circularize = args.circularize,
                               t0=launch, tof = tof, vinf=vinf, 
                               add_vinf_dep = args.add_dep_vinf, add_vinf_arr = args.add_arr_vinf, 
                               multi_objective = False)
    mga_1dsm_multi = None

    if multi_obj:
      mga_1dsm_multi = mga_1dsm_transx(seq=seq, 
                                       dep_altitude = args.dep_altitude, 
                                       arr_altitude = args.arr_altitude, 
                                       circularize = args.circularize,
                                       t0=launch, tof = tof, vinf = vinf, 
                                       add_vinf_dep = args.add_dep_vinf, add_vinf_arr = args.add_arr_vinf, 
                                       multi_objective = multi_obj)
    solutions = []
    # if sol_mga != None:
    #   t0 = sol_mga[0]
    #   T = sol_mga[1] / 365.25
    #   launch = [epoch(t0 - 30), epoch(t0 + 30)]
    #   tof = [0.1, T * 1.25]
    for i in range(nTrialMGA1DSM):
      # mga_1dsm.avoid = solutions
      op = optimizer(mga_1dsm, nTrial=5, gen=1000, mf=1000, mr = 2)
      sol_mga_1dsm = op.run_once()
      if sol_mga_1dsm != None:
        solutions.append(sol_mga_1dsm)
      
      if multi_obj:
        op = optimizer(mga_1dsm_multi, nTrial=1, gen=4000, mf = 100, mr = 10)
        sol_mga_multi = op.run_once(sol_mga_1dsm, True)

    if (len(solutions) > 0):
      sol_mga_1dsm = min(solutions, key=lambda x: mga_1dsm.calc_objective(x))
except Exception, e:
  logging.error(e, exc_info = True)
finally:
  if sol_mga != None:
    mga.pretty(sol_mga)
  if sol_mga_1dsm != None:
    mga_1dsm.pretty(sol_mga_1dsm)

