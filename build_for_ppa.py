#!/usr/bin/env python

"""
Dirty script for building package for PPA
by WangLu
2011.01.13

modified for pdf2htmlEX
2012.08.28
"""


import os
import sys
import re
import time

print 'Generating version...'

try:
    rev = open('.git/refs/heads/devv').read()[:5]
except:
    print 'Cannot get revision number'
    sys.exit(-1)

today_timestr = time.strftime('%Y%m%d%H%M')
package='pdf2htmlex'
projectname='pdf2htmlEX'
try:
    version = re.findall(r'set\(PDF2HTMLEX_VERSION\s*"([^"]*)"\)', open('CMakeLists.txt').read())[0]
except:
    print 'Cannot get package name and version number'
    sys.exit(-1)

deb_version = version+'-1~git'+today_timestr+'r'+rev
full_deb_version = deb_version+'-0ubuntu1'

#check if we need to update debian/changelog
if re.findall(r'\(([^)]+)\)', open('debian/changelog').readline())[0] == full_deb_version:
    print
    print 'No need to update debian/changelog, skipping'
else:
    print
    print 'Writing debian/changelog'
    if os.system('dch -v "%s"' % (full_deb_version,)) != 0:
        print 'Failed when updating debian/changelog'
        sys.exit(-1)

print
print 'Building...'
# handling files
if os.system('(rm CMakeCache.txt || true) && cmake . && make dist') != 0:
    print 'Failed in creating tarball'
    sys.exit(-1)

orig_tar_filename = package+'-'+version+'.tar.bz2'
if os.system('test -e %s && cp %s ../build-area' % (orig_tar_filename, orig_tar_filename)) != 0:
    print 'Cannot copy tarball file to build area'
    sys.exit(-1)

deb_orig_tar_filename = package+'_'+deb_version+'.orig.tar.bz2'

try:
    os.chdir('../build-area')
except:
    print 'Cannot find ../build-area'
    sys.exit(-1)

# remove old dir
os.system('rm -rf %s' % (package+'-'+version,))

if os.system('mv %s %s && tar -xvf %s' % (orig_tar_filename, deb_orig_tar_filename, deb_orig_tar_filename)) != 0:
    print 'Cannot extract tarball'
    sys.exit(-1)

try:
    os.chdir(package+'-'+version)
except:
    print 'Cannot enter project dir'
    sys.exit(-1)

os.system('cp -r ../../%s/debian .' % (projectname,))

# building
if os.system('debuild -S -sa') != 0:
    print 'Failed in debuild'
    sys.exit(-1)

print
sys.stdout.write('Everything seems to be good so far, upload?(y/n)')
sys.stdout.flush()
ans = raw_input().lower()
while ans not in ['y', 'n']:
    sys.stdout.write('I don\'t understand, enter \'y\' or \'n\':')
    ans = raw_input().lower()

if ans == 'n':
    print 'Skipped.'
    sys.exit(0)
   
if os.system('dput ppa:coolwanglu/%s ../%s' % (package, package+'_'+full_deb_version+'_source.changes')) != 0:
    print 'Failed in uploading by dput'
    sys.exit(-1)

print 'Build area not cleaned.'
print 'All done. Cool!'



