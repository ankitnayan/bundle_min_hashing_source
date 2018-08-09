
FILE_RELPATHS = 'data/FlickrLogos-v2/trainvalset.relpaths.txt' # already present in Flickr dataset
KEYS_DIR = 'data/FlickrLogos-v2/keys_train/' # Path of folder containing key files

f = open(FILE_RELPATHS, 'r')

total_count=0

for line in f.readlines():
    name_parts = line.rstrip().split('/')
    brand = name_parts[2]

    image_number = name_parts[3][:-4]
    key_file = KEYS_DIR + brand + "/" + image_number + ".key"

    try:
        line_count = sum(1 for line in open(key_file))-1
        total_count = total_count + line_count
        print brand, image_number, line_count
    except:
        print "error"

print "Total Keypoints = ", total_count
