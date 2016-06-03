#!/bin/bash
OrbiterKEP \
    --planets "earth,venus,venus,earth,jupiter,saturn" \
    --launch 19970101T000000,19971231T000000 \
    --tof-min 0.1 --tof-max 7 \
    --vinf-max 10 \
    --omit-dep-vinf \
    --algos-multi-obj nsge2 \
    --algos-single-obj mbh_cs,pso,bee_colony,ms_jde,jde_13 \
    --opt-gen 150000 \
    $@
if [ ]; then
     --algos-single-obj jde,ms_jde,mbh_cs \
     --algos-single-obj jde_11,jde_13,jde_15,jde_17 \
     --algos-single-obj de_1220,mbh_cs,sa_corana,cmaes,pso \
     --algos-single-obj jde,de_1220,mde_pbx \
     --algos-single-obj cmaes \
     --algos-single-obj mbh_cs \
     --algos-single-obj pso \
     --algos-single-obj bee_colony \
     --algos-single-obj sga_gray \
     --algos-single-obj sa_corana
fi

