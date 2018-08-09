import os, sys
import ctypes
import tables
import numpy as np
import numpy.random as npr

INPUT_H5_FILENAME = './clst_1M_iter10.h5'
OUTPUT_TXT_FILENAME = './Dictionary1M_iter10.txt'

pnts_fobj = tables.open_file(INPUT_H5_FILENAME, 'r')
for pnts_obj in pnts_fobj.walk_nodes('/', classname = 'Array'):
    break

N = pnts_obj.shape[0]
D = pnts_obj.shape[1]


np.savetxt(OUTPUT_TXT_FILENAME, pnts_obj[:], delimiter=' ')
		
print type(pnts_obj)
print N,D,pnts_obj.atom.dtype
