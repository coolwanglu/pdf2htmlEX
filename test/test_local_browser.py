#!/usr/bin/env python

import unittest
import os
import subprocess
import shutil

from PIL import Image, ImageChops
from selenium import webdriver
from test import Common

class test_local_browser(Common, unittest.TestCase):
    TTFAUTOHINT = 'ttfautohint'
    TEST_DATA_DIR = os.path.join(Common.TEST_DIR, 'test_local_browser')

    DEFAULT_PDF2HTMLEX_ARGS = [
        '--external-hint-tool', 'ttfautohint',
        '--fit-width', 800,
        '--last-page', 1,
        '--correct-text-visibility', 1,
        '--embed', 'fi', # avoid base64 to make it faster
    ]

    BROWSER_WIDTH=800
    BROWSER_HEIGHT=1200

    @classmethod
    def setUpClass(cls):
        exit_code = subprocess.call([cls.TTFAUTOHINT, '--version'])
        assert (exit_code == 0), 'Cannot execute ' + cls.TTFAUTOHINT
        cls.browser = webdriver.Firefox()
        cls.browser.maximize_window()
        size = cls.browser.get_window_size()
        assert ((size['width'] >= cls.BROWSER_WIDTH) and (size['height'] >= cls.BROWSER_HEIGHT)), 'Screen is not large enough'
        cls.browser.set_window_size(cls.BROWSER_WIDTH, cls.BROWSER_HEIGHT)

    @classmethod
    def tearDownClass(cls):
        cls.browser.quit()

    def run_test_case(self, filename, pdf2htmlEX_args=[]):
        basefilename, extension = os.path.splitext(filename)
        htmlfilename = basefilename + '.html'
        pngfilename = basefilename + '.png'

        self.assertEquals(extension.lower(), '.pdf', 'Input file is not PDF')

        pdf2htmlEX_args = self.DEFAULT_PDF2HTMLEX_ARGS \
            + list(pdf2htmlEX_args) + [
                os.path.join(self.TEST_DATA_DIR, filename),
                htmlfilename
            ]
        
        result = self.run_pdf2htmlEX(pdf2htmlEX_args)
        self.assertIn(htmlfilename, result['output_files'], 'HTML file is not generated')

        png_out_dir = os.path.join(self.cur_temp_dir, 'png_out')
        os.mkdir(png_out_dir)

        pngfilename_out_fullpath = os.path.join(png_out_dir, pngfilename)
        pngfilename_raw_fullpath = os.path.join(self.TEST_DATA_DIR, pngfilename)

        self.generate_image(os.path.join(self.cur_output_dir, htmlfilename), pngfilename_out_fullpath)


        if self.GENERATING_MODE:
            shutil.copy(pngfilename_out_fullpath, pngfilename_raw_fullpath)
        else:
            new_img = Image.open(pngfilename_out_fullpath)
            original_img = Image.open(pngfilename_raw_fullpath)

            diff_img = ImageChops.difference(original_img, new_img);

            if diff_img.getbbox() is not None:
                if self.SAVE_TMP:
                    # save the diff image
                    # http://stackoverflow.com/questions/15721484/saving-in-png-using-pil-library-after-taking-imagechops-difference-of-two-png
                    diff_img.convert('RGB').save(os.path.join(png_out_dir, basefilename + '.diff.png'))
                self.fail('PNG files differ')

    def generate_image(self, html_file, png_file):
        self.browser.get('file://' + html_file)
        self.browser.save_screenshot(png_file)

    def test_basic_text(self):
        self.run_test_case('basic_text.pdf')

    def test_geneve_1564(self):
        self.run_test_case('geneve_1564.pdf')

    def test_text_visibility(self):
        self.run_test_case('text_visibility.pdf')

