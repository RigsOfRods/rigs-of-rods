#!/usr/bin/python
# This source file is part of Rigs of Rods
# Copyright 2015 Artem Vorotnikov

# For more information, see https://rigsofrods.org

# Rigs of Rods is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3, as
# published by the Free Software Foundation.

# Rigs of Rods is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.

"""Recursively zips all folders in a given directory"""

import argparse
import os
import pathlib
import shutil
import zipfile


def zipper(src, dst):
    """Zips all files in a given directory"""
    zipfile_object = zipfile.ZipFile("%s.zip" % (dst), "w", zipfile.ZIP_DEFLATED)
    abs_src = os.path.abspath(src)
    for dirname, subdirs, files in os.walk(src):
        for filename in files:
            absname = os.path.abspath(os.path.join(dirname, filename))
            arcname = absname[len(abs_src) + 1:]
            zipfile_object.write(absname, arcname)
        print('Created %s.zip' % (dst))
    zipfile_object.close()


def recursive_zipper(workdir, zipdir):
    """Recursively zips all folders in a given directory"""
    if os.path.exists(zipdir):
        shutil.rmtree(zipdir)
    os.mkdir(zipdir)

    workdir_contents = [x for x in pathlib.Path(workdir).iterdir() if x.is_dir()]

    for i in range(len(workdir_contents)):
        zipper(str(workdir_contents[i]),
               os.path.join(zipdir,
                            os.path.basename(str(workdir_contents[i]))))

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("work_directory", type=str, default="./resources",
                        help="Directory for zipping")
    parser.add_argument("out_directory", type=str, default="./resources_zip",
                        help="Output directory")
    args = parser.parse_args()

    recursive_zipper(args.work_directory, args.out_directory)
