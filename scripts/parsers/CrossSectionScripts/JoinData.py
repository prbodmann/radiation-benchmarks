#!/usr/bin/env python

import argparse
import csv
import re
from datetime import datetime

"""
csv1        - normal Parser.py output file
del1        - delimiter for csv1
csv2        - calcCrossSection.py output file (it will only be used for check timestamp)
del2        - delimiter for csv2
outputFile  - obvious outputFile
"""
def joinFiles(csv1, del1, csv2, del2, outputFile):
    csvFileOne = open(csv1)
    csvFileTwo = open(csv2)
    readerOne = csv.DictReader(csvFileOne, delimiter=del1)
    readerTwo = csv.DictReader(csvFileTwo, delimiter=del2)

    outputCsv = open(outputFile, 'w')
    writer = csv.DictWriter(outputCsv, fieldnames=readerOne.fieldnames)
    writer.writeheader()

    #it is not possible iterate a reader twice, so
    #copy it to a list first
    listCsv1 = [i for i in readerOne]
    listCsv2 = [i for i in readerTwo]

    for i in listCsv1:
        logFileName = i["logFileName"]
        #process data
        #2016_12_13_19_00_34_cudaDarknet_carol-k402.log
        m = re.match("(\d+)_(\d+)_(\d+)_(\d+)_(\d+)_(\d+)_(.*)_(.*).log", logFileName)
        if m:
            year = m.group(1)
            month = m.group(2)
            day = m.group(3)
            hour = m.group(4)
            minutes = m.group(5)
            second = m.group(6)
            benchmark = m.group(7)

            # assuming microsecond = 0
            currDate = datetime(int(year), int(month), int(day), int(hour), int(minutes), int(second))
            for j in listCsv2:
                startDate = j["start timestamp"]
                endDate = j["end timestamp"]
                startDate = datetime.strptime(startDate, "%c")
                endDate = datetime.strptime(endDate, "%c")
                if startDate <= currDate <= endDate:
                    writer.writerow(i)
    #finishing
    csvFileOne.close()
    csvFileTwo.close()
    outputCsv.close()


def parse_args():
    """Parse input arguments."""

    parser = argparse.ArgumentParser(description='Parser to select good runs with good crossections')
    # parser.add_argument('--gold', dest='gold_dir', help='Directory where gold is located',
    #                     default=GOLD_DIR, type=str)
    parser.add_argument('--csv1', dest='csv1',
                        help='Where first csv file is located')
    parser.add_argument('--del1', dest='del1',
                        help='delimiter for csv 1', default=',')

    parser.add_argument('--csv2', dest='csv2',
                        help='Where second csv file is located, it must be the file which have the start and end timestamp')

    parser.add_argument('--del2', dest='del2',
                        help='delimiter for csv 2', default=',')

    parser.add_argument('--out', dest='output_file', help='Output file')

    args = parser.parse_args()

    return args

###########################################
# MAIN
###########################################'

if __name__ == '__main__':
    args = parse_args()
    csv1 = str(args.csv1)
    csv2 = str(args.csv2)
    del1 = str(args.del1)
    del2 = str(args.del2)
    outF = str(args.output_file)
    joinFiles(csv1, del1, csv2, del2, outF)