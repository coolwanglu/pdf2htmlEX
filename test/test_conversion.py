#!/usr/bin/env python

import unittest
import os
import subprocess

from PIL import Image, ImageChops
from test import Common

class T(Common, unittest.TestCase):
    GENERATING_MODE = os.environ.get('P2H_TEST_GEN')

    WKHTML2IMAGE = 'wkhtmltoimage'
    TTFAUTOHINT = 'ttfautohint'
    TEST_DATA_DIR = os.path.join(Common.TEST_DIR, 'test_conversion')

    DEFAULT_PDF2HTMLEX_ARGS = [
        '--external-hint-tool', 'ttfautohint',
        '--fit-width', 800,
        '--last-page', 1,
        '--correct-text-visibility', 1,
    ]

    DEFAULT_WKHTML2IMAGE_ARGS = [
        '-f', 'png',
        '--height', 600,
        '--width', 800,
        '--quality', 0,
        '--quiet'
    ]  

    @classmethod
    def setUpClass(cls):
        subprocess.check_call([cls.WKHTML2IMAGE, '--version'])
        subprocess.check_call([cls.TTFAUTOHINT, '--version'])

    def run_test_case(self, filename, pdf2htmlEX_args=[], wkhtml2image_args=[]):
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

        wkhtml2image_args = [self.WKHTML2IMAGE] \
            + self.DEFAULT_WKHTML2IMAGE_ARGS \
            + list(wkhtml2image_args) + [
                os.path.join(self.cur_output_dir, htmlfilename),
                pngfilename_out_fullpath
            ]

        return_code = subprocess.call(list(map(str, wkhtml2image_args)))
        self.assertEquals(return_code, 0, 'cannot execute ' + self.WKHTML2IMAGE)

        if self.GENERATING_MODE:
            shutil.copy(pngfilename_out_fullpath, pngfilename_raw_fullpath)
        else:
            original_img = Image.open(pngfilename_raw_fullpath)
            new_img = Image.open(pngfilename_out_fullpath)

            diff_img = ImageChops.difference(original_img, new_img);

            if diff_img.getbbox() is not None:
                if self.SAVE_TMP:
                    # save the diff image
                    # http://stackoverflow.com/questions/15721484/saving-in-png-using-pil-library-after-taking-imagechops-difference-of-two-png
                    diff_img.convert('RGB').save(os.path.join(png_out_dir, basefilename + '.diff.png'))
                self.fail('PNG files differ')

    def test_basic_text(self):
        self.run_test_case('basic_text.pdf', 
            wkhtml2image_args=[
                '--crop-x', 180,
                '--crop-y', 150,
                '--crop-w', 220,
                '--crop-h', 260
            ])

    def test_geneve_1564(self):
        self.run_test_case('geneve_1564.pdf', wkhtml2image_args=['--height', 1100])

    def test_text_visibility(self):
        self.run_test_case('text_visibility.pdf', wkhtml2image_args=['--height', 1200])

