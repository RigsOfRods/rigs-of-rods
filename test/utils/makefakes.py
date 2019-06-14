# Reads build output from MSVC and generates missing fake elements
#   * 'unresolved external symbol' -> adds symbol to fake header (creates new one if needed)
#   * 'no such file or directory' -> creates new fake header
# Tested on MSWindows 7 (x64) & 10 (x64) with CPython 3.7

# Usage: fill `ror_dir` path below and put file `buildiout.txt` (containing MSVC build log) to the working directory.

import os
import re
import sys
import pathlib

#ror_dir = 'd:\\Source\\rigs-of-rods'
ror_dir = 'c:\\users\\XXXXX\\Source\\rigs-of-rods'
test_dir = os.path.join(ror_dir, 'test')

def create_new_fake_and_proxy(origname, fakename):
    """
    origname = header filename
    fakepath = full path to fake header
    """
    proxypath=None
    incl_line=None
    try:
        # Determine source path of header
        #  https://stackoverflow.com/questions/2186525/how-to-use-glob-to-find-files-recursively
        found = pathlib.Path(os.path.join(test_dir, '../source/main')).glob('**/{}'.format(origname))
        for foundfile in found:
            foundfile = foundfile.relative_to(ror_dir)
            foundfile = os.path.normpath(foundfile)
            foundfile = pathlib.Path(foundfile).as_posix() # Necessary on MSWindows
            foundfile = foundfile.replace(origname.lower(), origname) # Necessary on MSWindows
            incl_line = '#include "../../{}"'.format(foundfile)
        
        # Create the fake header   
        fakepath = os.path.join(test_dir, 'fakes', fakename);
        with open(fakepath, 'wt') as nf:
            nf.write("#pragma once\n\n");
            nf.write(incl_line)
            nf.write("\n\n")
            nf.write("#ifdef ROR_FAKES_IMPL\n")
            nf.write("#endif // ROR_FAKES_IMPL\n")
        
        # Create a proxy header        
        proxypath = os.path.join(test_dir, 'proxies', origname)
        with open(proxypath, 'wt') as pf:
            pf.write("// Test proxy\n#pragma once\n#include \"../fakes/{}\"\n".format(fakename))
    
    except:
        print("ERROR CREATING NEW FAKE/PROXY")
        print("\torigname: {}".format(origname))
        print("\tfakename: {}".format(fakename))
        print("\texception: {}".format(sys.exc_info()[1]))       

# ------ main program ------

with open("buildout.txt", 'r') as f:
    for  line in f:
        # blank lines -> ignore silently
        if line.strip() == '':
            continue
    
        # missing includes -> generate it and continue
        if "C1083" in line:
            pattern = re.compile(r"C1083:.*'(.*)'"); # https://regex101.com/
            origname = pattern.search(line).group(1)
            fakename = "{}Fake.h".format(os.path.splitext(origname)[0])
            create_new_fake_and_proxy(origname, fakename)
            continue
    
        # unsupported lines -> print warning and continue
        if not ("LNK2019" in line or "LNK2001" in line or "__cdecl" in line):
            print("UNSUPPORTED: {}".format(line))
            continue
        
        # get the symbol
        pattern = re.compile(r'[^\"]*\"([^\"]*)\"'); # https://regex101.com/
        res = pattern.search(line)
        symbol = res.group(1)
        
        # tidy up the symbol
        subs = {
            'class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >': 'std::string',
            'class Ogre::Vector<3,float>': 'Ogre::Vector3',
            '__cdecl ': '',
            'public: ': '',
            'protected: ': '',
            'private: ': '',
            'virtual ': '',
            'static ': '',  
        }
        for src, dst in subs.items():
            symbol = symbol.replace(src, dst)
        if symbol[0:6] == "class ":
            symbol = symbol[6:]
        if symbol[0:7] == "struct ":
            symbol = symbol[7:]   
                    
        # determine fake name
        tmp = symbol
        tmp = tmp.replace('RoR::', '')
        tmp = tmp.replace('Utils::', '')
         
        prefix = ''
        if 'GUI::' in tmp:
            prefix = 'GUI_'
            tmp = tmp.replace('GUI::', '')
        if 'RigDef::' in tmp:
            prefix = 'RigDef_'
            tmp = tmp.replace('RigDef::', '')            
        pattern_ctor_dtor = re.compile(r'^([^:]*)::[\S]*\(.*') # https://regex101.com/
        pattern_method = re.compile(r'^([\S]*)([\s\*]*) ([^:]*)::[\S]*\(.*') # https://regex101.com/
        res = pattern_method.search(tmp)
        fakeclass = ''
        returntype = None        
        if res:
            returntype = res.group(1)
            if res.group(2) != '':
                returntype = '*'        
            fakeclass = res.group(3)       
            print("DBG    method: {}, symbol: {}, returntype='{}'".format(fakeclass, symbol, returntype))
        else:
            res = pattern_ctor_dtor.search(tmp)
            if res:
                fakeclass = res.group(1)
                print("DBG ctor/dtor: {}, symbol: {}".format(fakeclass, symbol))
            else:
              print("CANNOT DETERMINE FAKE: {} (line: {})".format(tmp, line))
              continue
       
        namesub = {
            'EngineSim': 'BeamEngine',
            'ActorManager': 'BeamFactory',
            'ActorSpawner': 'RigSpawner',
            'Actor': 'Beam',
            'SimController': 'RoRFrameListener',
            'MpClientList': 'MultiplayerClientList',
            'GfxEnvmap': 'EnvironmentMap',
            'Differential': 'Differentials',
            'RailGroup': 'SlideNode',
            'RailSegment': 'SlideNode' 
        }
        for cname, fname in namesub.items():
            if cname in fakeclass:
                fakeclass = fname
                
        fakeclass = fakeclass.replace('const', '') # FIXME: The regex should cover this
        fakeclass = fakeclass.replace('&', '').strip()  # FIXME: The regex should cover this

        origname = "{}{}.h".format(prefix, fakeclass)
        fakename = "{}{}Fake.h".format(prefix, fakeclass)                
        fakepath = os.path.join(test_dir, 'fakes', fakename)
        
        if not os.path.isfile(fakepath):
            create_new_fake_and_proxy(origname, fakename)
         
        # add the symbol to the fake
        fakelines = []
        with open(fakepath, 'rt') as ff:
            fakelines = ff.readlines()
                           
        if returntype == 'bool':
            body = '{return false;}'
        elif returntype == 'float':
            body = '{return 0.f;}'
        elif returntype == 'int':
            body = '{return 0.f;}'
        elif returntype == '*':
            body = '{return nullptr;}'
        elif returntype == 'std::string':
            body = '{return "";}'
        elif returntype == 'Ogre::Vector3':
            body = '{return  Ogre::Vector3::ZERO;}'
        elif returntype == 'void' or returntype is None:
            body = '{}'                        
        else:
            body = '{return '+returntype+'();}'        
        
        with open(fakepath, 'wt') as ff:
            for fakeline in fakelines:
                if "#endif // ROR_FAKES_IMPL" in fakeline:
                    ff.write("    {} {}\n".format(symbol, body))
                ff.write(fakeline) # newline is embedded
                                                                 
