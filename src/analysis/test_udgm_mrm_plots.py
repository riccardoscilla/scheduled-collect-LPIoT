import matplotlib.pyplot as plt
import pandas as pd
from collections import defaultdict
import numpy as np
import math

plt.rcParams.update({'font.size': 15})

def getValues(x, p):
    dc_values = []
    pdr_values = []
    
    dc_std = []
    pdr_std = []

    for i in x:
        folder = "results/mrm/test_mrm_"+p+"_changeP/"

        df = pd.read_csv(folder+"dc"+i+".csv", delimiter="\t")

        dc = []
        for index, row in df.iterrows():
            dc.append(row['dc'])
        dc_values.append(np.mean(dc))
        dc_std.append(np.std(dc))

        folder = "results/mrm/test_mrm_"+p+"_changeP/"
        df = pd.read_csv(folder+"pdr"+i+".csv", delimiter="\t")

        pdr = []
        for index, row in df.iterrows():
            pdr.append(row['pdr'])
        pdr_values.append(np.mean(pdr))
        pdr_std.append(np.std(pdr))

    return dc_values, pdr_values, dc_std, pdr_std



fig, (ax1,ax2) = plt.subplots(ncols=2, figsize=(14,7), constrained_layout=True)

x = ["3","7","10","15"]

dc10, pdr10, dc10_error, pdr10_error = getValues(x,"10")
dc20, pdr20, dc20_error, pdr20_error = getValues(x,"20")
dc50, pdr50, dc50_error, pdr50_error = getValues(x,"50")

ax1.set_title('PDR per P and N value')
ax1.set_ylabel('Average PDR (%)')
ax1.set_xlabel('P value')
width = 0.25
xpos = np.arange(len(x))
ax1.bar(xpos-width, pdr10, width, yerr=pdr10_error, capsize=3, label="10")
ax1.bar(xpos, pdr20, width, yerr=pdr20_error, capsize=3, label="20")
ax1.bar(xpos+width, pdr50, width, yerr=pdr50_error, capsize=3, label="50")
ax1.set_xticks(xpos)
ax1.set_xticklabels(x)
ax1.grid(axis="y")
ax1.legend(title="N value",loc=4)

ax2.set_title('DC per P and N value')
ax2.set_ylabel('Average DC (%)')
ax2.set_xlabel('P value')
ax2.grid(axis="y")
ax2.errorbar(x,dc10,yerr=dc10_error,fmt='-o', label="10")
ax2.errorbar(x,dc20,yerr=dc20_error,fmt='-o', label="20")
ax2.errorbar(x,dc50,yerr=dc50_error,fmt='-o', label="50")
ax2.legend(title="N value",loc=4)

plt.show()
fig.savefig('images/mrm.png')