import sys, os
import numpy as np
from math import log10, floor

batch='''#!/bin/bash
#SBATCH -N 1   # node count
#SBATCH --ntasks-per-node=1
#SBATCH --mem=4gb
#SBATCH -t 20:00:00
##SBATCH --mail-type=begin
##SBATCH --mail-type=end
##SBATCH --mail-user=smsharma@princeton.edu
#SBATCH -p hepheno

cd /group/hepheno/smsharma/Dark-Showers
source env.sh
cd /group/hepheno/smsharma/Dark-Showers/gen
source activate venv_py27

./monojet.exe -m lhe -Zprime -w -i '''

# rinv_ary = np.linspace(0.1,1.,5)
rinv_ary = [0.01,0.1,.2,.3,.4,.5,.6,.7,.8,.9, .98, 0.99, 1.0]

# for MZp in [500, 1000, 3000]:
masses = np.arange(1000,4100,100)
for MZp in masses:
    for rinv in rinv_ary:    
        # fname_tag = "/group/hepheno/smsharma/Dark-Showers/MG5_aMC_v2_5_2/bin/DM_Zprime_" + str(MZp) + "/Events/run_01/unweighted_events.lhe -metmin 0 -n 200000 -phimass 10 -lambda 5 -v "
        fname_tag = "/group/hepheno/hlou/Zprime_" + str(MZp) + ".lhe -metmin 0 -n 50000 -phimass 10 -lambda 5 -v "
        rinv_tag = "-inv " + str(rinv)
        out_tag = "-o " + "ZprimeEventsFixed/Zprime_MZ_"+str(MZp)+"_gX_1_gq_0p1_rinv_" + str(rinv)
        tag = fname_tag + " " + rinv_tag + " " + out_tag
        print tag
        batchn = batch + tag
        fname = "batch/run_monojet_"+str(MZp)+"_"+str(rinv)+".batch"
        f=open(fname, "w")
        f.write(batchn)
        f.close()
        os.system("sbatch "+fname);