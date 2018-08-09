import os, math
import tables
from array import *
import numpy as np


MAX_POINTS_NUM = 11888319

KEYS_DIR = './data/FlickrLogos-v2/keys_train/' #Folder contains key files
OUTPUT_FILE = 'pnts_float_rootsift.h5' #Output file name


my_list  = []
myc_array = np.empty([MAX_POINTS_NUM, 128],'f4')

count_dir=-1

points_count=0;


for dirName, subdirList, fileList in os.walk(KEYS_DIR):
	#print('Found directory: %s' % dirName)

	count_dir += 1
	#    print dirName
	if dirName != KEYS_DIR:

		count_file=0

		for fname in fileList:

			count_file += 1
			#print dirName+"/"+fname
			f = open(dirName+"/"+fname,'r')
			ignore_first=0

			del my_list[:]
			flag=0
			for line in f:

				if ignore_first==0: # ignore first line of key file which contains size of image
					ignore_first += 1
					continue;
				else:
					#print line[0:-1]
					my_list.append([float(x) for x in line.split()[5:]]) # we start from 6th since the first 5 contains x, y, angle, scale and octave



			for i in range(0, len(my_list)):

				if (MAX_POINTS_NUM - points_count) != 0:
					for j in range(0,128):
						myc_array[points_count][j] = my_list[i][j]
						points_count += 1
				else:
					flag=1
					break

			if flag==1:
			break

			print str(count_dir)+" : "+dirName+"\t"+str(count_file)+" : "+fname
			f.close()

		if len(subdirList) > 0:
		del subdirList[:]

		print "Wrting list of "+str(count_file)+" files to array ...."




		print "Num Points written to array : "+str(points_count)


#print myc_array[0]
print "Writing data to file ...."
pnts_fobj = tables.open_file(OUTPUT_FILE,'w')
pnts_fobj.create_array(pnts_fobj.root, 'pnts', myc_array)
pnts_fobj.close()
print "pnts.hd5 file made !!!"
del myc_array

