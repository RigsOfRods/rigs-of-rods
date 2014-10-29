#!/usr/bin/env python
# python 2.6 required (for os.path.relpath)

# Generate *.list files with CMake SOURCE_GROUP from *.vcproj files
# goes through recursively from ../build directory and put results into ../
# right now all projects except RoR is ignored


import xml.dom.minidom, os, filecmp, sys

headers = []
source = []
alllines = []
currentFolder = ""

def addSourceOrHeader(line):
    #print line
    if line.endswith("CMakeLists.txt"):
        return
    if line.endswith('.h'):
        headers.append(line + '\n')
    else:
        source.append(line + '\n')

def get_a_document(name):
    return xml.dom.minidom.parse(name)

def parseFilter(_baseFilterNode, _filterFolder):
    lines = []
    filterName = _filterFolder
    if _filterFolder != "":
        filterName += "\\\\"
    if _baseFilterNode.attributes.getNamedItem("Name") != None:
        filterName += str(_baseFilterNode.attributes.getNamedItem("Name").nodeValue)
    lines.append("SOURCE_GROUP(\"" + filterName + "\" FILES\n")
    for filterNode in _baseFilterNode.childNodes:
        if filterNode.nodeType != filterNode.TEXT_NODE:
            if filterNode.localName == "File":
                fileName = str(filterNode.attributes.getNamedItem("RelativePath").nodeValue)
                fileName = os.path.join(root, fileName)
                fileName = fileName.replace('\\','/')
                fileName = os.path.relpath(fileName, currentFolder)
                fileName = fileName.replace('\\','/')
                addSourceOrHeader("  " + fileName)
                lines.append("  " + fileName + "\n")
            if filterNode.localName == "Filter":
                linesFromFile = parseFilter(filterNode, filterName)
                for line in linesFromFile:
                    alllines.append( line )
    lines.append(")\n")
    return lines

def createFilesList(fileName, listName):

    print "Converting " + fileName
    doc = get_a_document(fileName)

    headers.append("set (HEADER_FILES\n")
    source.append("set (SOURCE_FILES\n")

    for rootNode in doc.childNodes:
        for subNode in rootNode.childNodes:
            if subNode.nodeType == subNode.ELEMENT_NODE and subNode.localName == "Files":
                linesFromFile = parseFilter(subNode, "")
                for line in linesFromFile:
                    alllines.append( line )

    headers.append(")\n")
    source.append(")\n")
    #remove ")" at start and add at end
    #lines.remove(")\n")
    #lines.append(")\n")

    FILE = open(listName, "w")
    FILE.writelines(headers)
    FILE.writelines(source)
    FILE.writelines(alllines)
    FILE.close()

    del headers[:]
    del source[:]
    del alllines[:]

def isIgnoredProject(name):
    if (not name.startswith("RoR")):
        return True
    ignores = ["Common", "api-docs", "INSTALL", "ALL_BUILD", "ZERO_CHECK", "PACKAGE"]
    for ignore in ignores:
        if name.startswith(ignore):
            return True
    return False

# ----------
dir_src = '../'

dir_solution = '../'
#try:
#  dir_solution = sys.argv[1]
#except:
#  print "Error: missing argument"
#  print "Usage: createCMakeLists <path_to_solution>"
#else:
if (True):
  for root, dirs, files in os.walk(dir_solution):
    for name in files:
      if name.endswith('.vcproj') and not isIgnoredProject(name):
          f_src = os.path.join(root, name)
          f_src = f_src.replace('\\','/')
          currentFolder = f_src #os.path.realpath(f_src)
          currentFolder = currentFolder.replace(name, "")
          currentFolder = currentFolder.replace('\\','/')
          
          currentFolder = os.path.join(dir_src, os.path.relpath(currentFolder, dir_solution))
          #currentFolder = os.path.realpath(currentFolder)
          #print currentFolder
          
          listName = f_src.replace(".vcproj", ".list")
          listName = os.path.relpath(listName, dir_solution)
          listName = os.path.join(dir_src, listName)
          #listName = os.path.realpath(listName)
          #print listName
          createFilesList(f_src, listName)

  print "Done"