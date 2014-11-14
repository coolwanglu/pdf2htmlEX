#!/usr/bin/env python

import unittest
import os
import sys
import tempfile
import shutil
import subprocess

class Common(object):
    SRC_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    TEST_DIR = os.path.join(SRC_DIR, 'test')
    DATA_DIR = os.path.join(SRC_DIR, 'share')
    PDF2HTMLEX_PATH = os.path.join(SRC_DIR, 'pdf2htmlEX')
    
    def setUp(self):
        self.cur_temp_dir = tempfile.mkdtemp(prefix='pdf2htmlEX_test')
        self.cur_data_dir = os.path.join(self.cur_temp_dir, 'share')
        self.cur_output_dir = os.path.join(self.cur_temp_dir, 'out')
        os.mkdir(self.cur_data_dir)
        os.mkdir(self.cur_output_dir)
        
        # filter manifest
        with open(os.path.join(self.DATA_DIR, 'manifest')) as inf:
            with open(os.path.join(self.cur_data_dir, 'manifest'), 'w') as outf:
                ignore = False
                for line in inf:
                    if ignore:
                        if line.startswith('#TEST_IGNORE_END'):
                            ignore = False
                    elif line.startswith('#TEST_IGNORE_BEGIN'):
                        ignore = True
                    else:
                        outf.write(line)

        # copy files
        shutil.copy(os.path.join(self.DATA_DIR, 'base.min.css'),
                os.path.join(self.cur_data_dir, 'base.min.css'))

    def tearDown(self):
        shutil.rmtree(self.cur_temp_dir, True)

    def run_pdf2htmlEX(self, args):
        """
        Execute the pdf2htmlEX with the specified arguments.

        :type args: list of values
        :param args: list of arguments to pass to executable. 
        :return: an object of relevant info
        """

        cmd = [self.PDF2HTMLEX_PATH,
               '--data-dir', self.cur_data_dir,
               '--dest-dir', self.cur_output_dir
              ]

        for val in args:
            cmd.append(str(val))

        return_code = subprocess.call(cmd)
        self.assertEquals(return_code, 0)

        files = os.listdir(self.cur_output_dir)
        files.sort()

        return { 
            'return_code' : return_code,
            'output_files' : files
        }
    

if __name__ == '__main__':
    if not os.path.isfile(Common.PDF2HTMLEX_PATH) or not os.access(Common.PDF2HTMLEX_PATH, os.X_OK):
        print >> sys.stderr, "Cannot locate pdf2htmlEX executable. Make sure source was built before running this test."
        exit(1)
    suites = []
    loader = unittest.TestLoader()
    for module_name in ['test_naming']:
        __import__(module_name)
        suites.append(loader.loadTestsFromModule(sys.modules[module_name]))

    failure_count = 0
    runner = unittest.TextTestRunner(verbosity=2)
    for suite in suites:
        result = runner.run(suite)
        failure_count += len(result.errors) + len(result.failures)

    exit(failure_count)
