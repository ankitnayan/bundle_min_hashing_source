import csv
from collections import Counter


INPUT_FILE_COLLISIONS = './results_sketches_logosonly.csv'
OUTPUT_FILE_CLASSIFIED = './results_classified_sketches_logosonly.csv'
NUM_QUERIES = 960 #running for 960 query logosonly images


write_csv = open(OUTPUT_FILE_CLASSIFIED, 'wb')
writer = csv.writer(write_csv)

with open(INPUT_FILE_COLLISIONS, 'rb') as csvfile:

	reader = csv.reader(csvfile, delimiter=',', quotechar='|')
	
	count_true = 0
	count_false = 0

	lines = list(reader)
	for row in lines:

		#returns histogram of files in the list of collision files
		file_counts = Counter(row[1:-1]) # in a row the first item is test file itself and the last item is blank due to last comma
		
		#print row[0], " --> ", file_counts.most_common()[:5]

		#get list of brand from list of top 5 files (based on histogram size or num collisions)
		list_brands = [x[0].split('/')[0] for x in file_counts.most_common()[:5]]
		
		#create histogram of brands and select top 5
		hist_brands = Counter(list_brands).most_common()[:5]

		#extracting ground truth of test file from filename
		ground_truth_brand = row[0].split('/')[0]

		classified_brand = ""
		classification_type = "False"

		# if no collision with any file return NF (Not Found)
		if len(hist_brands) == 0:
			classified_brand = "NF"
			
		# if only 1 brand found then classify test file to that brand
		elif len(hist_brands) == 1:
			classified_brand = hist_brands[0][0]

		# if 2 or more brands are found then the top brand has to be greater than 2nd top brand (majority)
		elif hist_brands[0][1]>hist_brands[1][1]:
			classified_brand = hist_brands[0][0]
		#else if top 2 brands have same size of histogram then classify as NF (since no majority)
		else:
			classified_brand = "NF"



		if ground_truth_brand==classified_brand:
			classification_type = "True"
			count_true += 1
		elif classified_brand=="no-logo" or classified_brand=="NF":
			classification_type = "NA"
		else:
			classification_type = "False"
			count_false += 1

		row2write = [row[0], ground_truth_brand, classified_brand, classification_type]
		writer.writerow(row2write)

	precision = count_true*1.0/(count_true+count_false)
	recall = count_true*1.0/NUM_QUERIES

	print "Precision: ",precision 
	print "Recall:    ", recall

write_csv.close()
