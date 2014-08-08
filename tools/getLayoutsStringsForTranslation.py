#!/usr/bin/env python

import os, filecmp, sys

alllines = []

def addTagsFromFile(fileName, outputFile):
	print "Parsing " + fileName
	file = open(fileName, 'r')
	count = 0

	line = file.readline()
	while line != "":
		count = count + 1
		leftPos = line.find("#{")
		while (leftPos != -1):
			rightPos = line.find("}", leftPos)
			tagString = line[leftPos + 2 : rightPos]
			outputFile.write("# GUI string\n")
			outputFile.write("# " + fileName[fileName.rfind("/") + 1 : ] + ":" + str(count) + "\n")
			outputFile.write("msgid \"" + tagString + "\"\n")
			outputFile.write("msgstr \"\"\n")
			outputFile.write("\n")
			
			leftPos = line.find("#{", rightPos)
		
		line = file.readline()
	file.close ()

def createPo(layoutsDir, outputFileName):
	outputFile = open(outputFileName, 'w')
	for root, dirs, files in os.walk(layoutsDir):
		for name in files:
			if name.endswith('.layout'):
				layoutFile = os.path.join(root, name)
				layoutFile = layoutFile.replace('\\','/')
				addTagsFromFile(layoutFile, outputFile)
	outputFile.close()
# ----------

layoutsDir = "../bin/resources/mygui"
outputFileName = "gui_strings.po"

if len(sys.argv) == 1:
	# keep default layoutsDir and output file name
	print "Using default paths"
	createPo(layoutsDir, outputFileName)
elif len(sys.argv) == 2:
	layoutsDir = sys.argv[1]
	createPo(layoutsDir, outputFileName)
elif len(sys.argv) == 3:
	layoutsDir = sys.argv[1]
	outputFileName = sys.argv[2]
	createPo(layoutsDir, outputFileName)
else:
	print "Usage: getLayoutsStringsForTranslation [path_to_layouts [output_file_name]]"
	print "    - if no path_to_layouts specified then \"../../contents/contents/source/layouts\" directory used."
	print "    - if no output_file_name specified then \"gui_strings.po\" used."
