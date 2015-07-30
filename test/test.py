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

    SAVE_TMP = bool(os.environ.get('P2H_TEST_SAVE_TMP'))
    GENERATING_MODE = bool(os.environ.get('P2H_TEST_GEN'))

    CANONICAL_TEMPDIR = '/tmp/pdf2htmlEX_test'
    
    def setUp(self):
        if not self.SAVE_TMP:
            self.cur_temp_dir = tempfile.mkdtemp(prefix='pdf2htmlEX_test')
        else:
            shutil.rmtree(self.CANONICAL_TEMPDIR, True)
            os.mkdir(self.CANONICAL_TEMPDIR)
            self.cur_temp_dir = self.CANONICAL_TEMPDIR

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
        shutil.copy(os.path.join(self.TEST_DIR, 'fancy.min.css'),
                os.path.join(self.cur_data_dir, 'fancy.min.css'))

    def tearDown(self):
        if not self.SAVE_TMP:
            shutil.rmtree(self.cur_temp_dir, True)

    def run_pdf2htmlEX(self, args):
        """
        Execute the pdf2htmlEX with the specified arguments.

        :type args: list of values
        :param args: list of arguments to pass to executable. 
        :return: an object of relevant info
        """

        args = [self.PDF2HTMLEX_PATH,
               '--data-dir', self.cur_data_dir,
               '--dest-dir', self.cur_output_dir
              ] + args

        with open(os.devnull, 'w') as fnull:
            return_code = subprocess.call(list(map(str, args)), stderr=fnull)

        self.assertEquals(return_code, 0, 'cannot execute pdf2htmlEX')

        files = os.listdir(self.cur_output_dir)

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

    all_modules = []
    all_modules.append(__import__('test_output'))
    all_modules.append(__import__('test_local_browser'))
    all_classes = ['test_output', 'test_local_browser']

    if bool(os.environ.get('P2H_TEST_REMOTE')):
        m = __import__('test_remote_browser')
        all_modules.append(m)
        all_classes += m.test_classnames

    test_names = []
    for name in sys.argv[1:]:
        test_names.append(name)
        if name.find('.') == -1:
            for m in all_classes:
                test_names.append(m + '.' + name)
    
    for module in all_modules:
        if len(test_names) > 0 and module.__name__ not in test_names:
            for n in test_names:
                try:
                    suites.append(loader.loadTestsFromName(n, module))
                except:
                    pass
        else:
            suites.append(loader.loadTestsFromModule(module))

    if len(suites) == 0:
        print >>sys.stderr, 'No test found'
        exit(1)

    failure_count = 0
    runner = unittest.TextTestRunner(verbosity=2)
    for suite in suites:
        result = runner.run(suite)
        failure_count += len(result.errors) + len(result.failures)

    exit(failure_count)
