import os
import multiprocessing


## Relative paths of images are added to this base path of flickr dataset
BASE_PATH = "./data/FlickrLogos-v2/"

## Folder where key files will be made
KEYS_FOLDER_BASE = "./data/FlickrLogos-v2/keys_train/"

## File containing relative paths of images - file already in flickr dataset
RELPATH_FILENAME = "./data/FlickrLogos-v2/trainvalset.relpaths.txt"

## Path to sift executable (keep space after path)
ROOTSIFT_PATH = "./rootSift/g64/sift "	

NUM_CORES = 4
TOTAL_IMAGES = 4280	# Num Images to train from the RELPATH_FILENAME

NUM_IMAGES_PER_CORE = TOTAL_IMAGES/NUM_CORES # make sure this is integer

try:
	os.stat(KEYS_FOLDER_BASE)
except:
	os.mkdir(KEYS_FOLDER_BASE)


# Worker threads
def worker(start, end):
	
	
	for line in open(RELPATH_FILENAME,'r').readlines()[start:end]:

	    name_parts = line.rstrip().split('/')
	    brand = name_parts[2]

	    image_number = name_parts[3][:-4]
	    rel_path= line.rstrip()

	    keys_folder = KEYS_FOLDER_BASE + brand + "/"

	    try:
		os.stat(keys_folder)
	    except:
		os.mkdir(keys_folder)    


	    jpg_path 		= BASE_PATH + rel_path
	    pgm_path 		= BASE_PATH + rel_path.split('.')[0] + ".pgm"
	    key_temp_path 	= BASE_PATH + rel_path.split('.')[0] + ".key"
	    key_final_path 	= keys_folder + image_number + ".key" 

	    cmd2pgm  = "./jpg2pgm " + jpg_path + " " + pgm_path
	    run_sift = ROOTSIFT_PATH + pgm_path
	    mv_key   = "mv " + key_temp_path + " " + key_final_path
	    rm_pgm 	 = "rm " + pgm_path

	    print brand, image_number

	    os.system(cmd2pgm)	# converts jpg to pgm needs executable of jpg2pgm.cpp
	    os.system(run_sift) # creates rootsift features (prepare rootsift executable first)
	    os.system(mv_key)	# moves key file made to the desired location
	    os.system(rm_pgm)	# deletes pgm files - comment this line if you want to create multiple versions of keypoints for testing



if __name__ == '__main__':
    
    procs = []

    for i in range(0, NUM_CORES):
        p = multiprocessing.Process(
                target=worker,
                args=(i*NUM_IMAGES_PER_CORE, (i+1)*NUM_IMAGES_PER_CORE))
        procs.append(p)
        p.start()


