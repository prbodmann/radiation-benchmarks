from Parser import Parser
import csv
import copy
import os
#build image
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from PIL import Image
import numpy as np

class ObjectDetectionParser(Parser):

    # precisionRecallObj = None
    _prThreshold = 0.5
    _detectionThreshold = 0.2

    # def __init__(self):
    #     Parser.__init__(self)
        # self.precisionRecallObj = PrecisionAndRecall.PrecisionAndRecall(self._prThreshold)

    _classes = ['__background__',
               'aeroplane', 'bicycle', 'bird', 'boat',
               'bottle', 'bus', 'car', 'cat', 'chair',
               'cow', 'diningtable', 'dog', 'horse',
               'motorbike', 'person', 'pottedplant',
               'sheep', 'sofa', 'train', 'tvmonitor']

    #overiding csvheader
    _csvHeader = ["logFileName", "Machine", "Benchmark", "SDC_Iteration", "#Accumulated_Errors",
                             "#Iteration_Errors", "gold_lines", "detected_lines", "wrong_elements", "x_center_of_mass",
                             "y_center_of_mass", "precision", "recall", "false_negative", "false_positive",
                             "true_positive", "abft_type", "row_detected_errors", "col_detected_errors",
                    "header"]

    # ["gold_lines",
    #     "detected_lines", "x_center_of_mass", "y_center_of_mass", "precision",
    #     "recall", "false_negative", "false_positive", "true_positive"]
    _goldLines = None
    _detectedLines = None
    _wrongElements = None
    _xCenterOfMass = None
    _yCenterOfMass = None
    _precision = None
    _recall = None
    _falseNegative = None
    _falsePositive = None
    _truePositive = None

    # only for darknet
    _abftType = None
    _rowDetErrors = None
    _colDetErrors = None

    def getBenchmark(self):
        return self._benchmark


    def _writeToCSV(self, csvFileName):
        # if not os.path.isfile(csvFileName) and self._abftType != 'no_abft' and self._abftType != None:
        #     self._csvHeader.extend(
        #         [])

        self._writeCSVHeader(csvFileName)

        try:

            csvWFP = open(csvFileName, "a")
            writer = csv.writer(csvWFP, delimiter=';')
            # ["logFileName", "Machine", "Benchmark", "imgFile", "SDC_Iteration",
            #     "#Accumulated_Errors", "#Iteration_Errors", "gold_lines",
            #     "detected_lines", "x_center_of_mass", "y_center_of_mass",
            #     "precision", "recall", "false_negative", "false_positive",
            #     "true_positive"]
            outputList = [self._logFileName,
                          self._machine,
                          self._benchmark,
                          self._sdcIteration,
                          self._accIteErrors,
                          self._iteErrors, self._goldLines,
            self._detectedLines,
            self._wrongElements,
            self._xCenterOfMass,
            self._yCenterOfMass,
            self._precision,
            self._recall,
            self._falseNegative,
            self._falsePositive,
            self._truePositive, self._abftType, self._rowDetErrors,
                self._colDetErrors,
            self._header
            ]

            # if self._abftType != 'no_abft' and self._abftType != None:
            #     outputList.extend([])

            writer.writerow(outputList)
            csvWFP.close()

        except:
            #ValueError.message += ValueError.message + "Error on writing row to " + str(csvFileName)
            print "Error on writing row to " + str(csvFileName)
            raise

    def relativeErrorParser(self):
        self._relativeErrorParser(self._errors["errorsParsed"])

    def localityParser(self):
        pass

    def jaccardCoefficient(self):
        pass

    def copyList(self, objList):
        temp = []
        for i in objList: temp.append(i.deepcopy())
        return temp



    def buildImageMethod(self, imageFile, rectangles, rectanglesFound):
        im = np.array(Image.open(imageFile), dtype=np.uint8)

        # Create figure and axes
        fig = plt.figure()
        axG = fig.add_subplot(221)
        axF = fig.add_subplot(222)
        # fig, ax = plt.subplots(1)

        # Display the image
        axG.imshow(im)
        axF.imshow(im)
        # Create a Rectangle patch
        for rG in rectangles:
            rect = patches.Rectangle((rG.left, rG.bottom), rG.width,
                                     rG.height, linewidth=1, edgecolor='r',
                                     facecolor='none')

            # Add the patch to the Axes
            axG.add_patch(rect)

        axG.title.set_text("gold")


            # ax.text(r.left, r.bottom - 2,
            #         'class ' + str(c) + ' prob ' + str(p),
            #         bbox=dict(facecolor='blue', alpha=0.5), fontsize=14,
            #         color='white')

        for rF in rectanglesFound:
            rectF = patches.Rectangle((rF.left, rF.bottom), rF.width,
                                     rF.height, linewidth=1, edgecolor='r',
                                     facecolor='none')
            axF.add_patch(rectF)
        axF.title.set_text("found")

        plt.show()