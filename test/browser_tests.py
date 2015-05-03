#!/usr/bin/env python

import os
import subprocess
import shutil
import unittest

from PIL import Image, ImageChops
from test import Common

class BrowserTests(Common):
    TEST_DATA_DIR = os.path.join(Common.TEST_DIR, 'browser_tests')

    DEFAULT_PDF2HTMLEX_ARGS = [
        '--fit-width', 800,
        '--last-page', 1,
        '--embed', 'fi', # avoid base64 to make it faster
    ]

    BROWSER_WIDTH=800
    BROWSER_HEIGHT=1200

    @classmethod
    def setUpClass(cls):
        pass

    @classmethod
    def tearDownClass(cls):
        pass

    def run_test_case(self, filename, pdf2htmlEX_args=[], page_must_load=True):
        basefilename, extension = os.path.splitext(filename)
        htmlfilename = basefilename + '.html'

        ref_htmlfolder = os.path.join(self.TEST_DATA_DIR, basefilename)
        ref_htmlfilename = os.path.join(ref_htmlfolder, htmlfilename)

        out_htmlfilename = os.path.join(self.cur_output_dir, htmlfilename)

        self.assertEquals(extension.lower(), '.pdf', 'Input file is not PDF')

        pdf2htmlEX_args = self.DEFAULT_PDF2HTMLEX_ARGS \
            + list(pdf2htmlEX_args) + [
                os.path.join(self.TEST_DATA_DIR, filename),
                htmlfilename
            ]

        result = self.run_pdf2htmlEX(pdf2htmlEX_args)
        self.assertIn(htmlfilename, result['output_files'], 'HTML file is not generated')

        if self.GENERATING_MODE:
            # copy generated html files
            shutil.rmtree(ref_htmlfolder, True)
            shutil.copytree(self.cur_output_dir, ref_htmlfolder)
            return

        png_out_dir = os.path.join(self.cur_temp_dir, 'png_out')
        os.mkdir(png_out_dir)

        pngfilename_out_fullpath = os.path.join(png_out_dir, basefilename + '.out.png')
        self.generate_image(out_htmlfilename, pngfilename_out_fullpath)
        out_img = Image.open(pngfilename_out_fullpath)

        pngfilename_ref_fullpath = os.path.join(png_out_dir, basefilename + '.ref.png')
        self.generate_image(ref_htmlfilename, pngfilename_ref_fullpath, page_must_load=page_must_load)
        ref_img = Image.open(pngfilename_ref_fullpath)

        diff_img = ImageChops.difference(ref_img, out_img);

        diff_bbox = diff_img.getbbox()
        if diff_bbox is not None:
            diff_size = (diff_bbox[2] - diff_bbox[0]) * (diff_bbox[3] - diff_bbox[1])
            img_size = ref_img.size[0] * ref_img.size[1]
            if self.SAVE_TMP:
                # save the diff image
                # http://stackoverflow.com/questions/15721484/saving-in-png-using-pil-library-after-taking-imagechops-difference-of-two-png
                diff_img.convert('RGB').save(os.path.join(png_out_dir, basefilename + '.diff.png'))
            self.fail('PNG files differ by <= %d pixels, (%f%% of %d pixels in total)' % (diff_size, 1.0*diff_size/img_size, img_size))

    @unittest.skipIf(Common.GENERATING_MODE, 'Do not auto generate reference for test_fail')
    def test_fail(self):
        # The HTML reference is generated manually, which mismatches the PDF
        # To test if the environment can detect any errors
        # E.g. when network is down, 404 message is shown for any HTML message
        with self.assertRaises(AssertionError):
            self.run_test_case('test_fail.pdf', page_must_load=False)

    def test_basic_text(self):
        self.run_test_case('basic_text.pdf')

    def test_geneve_1564(self):
        self.run_test_case('geneve_1564.pdf')

    def test_text_visibility(self):
        self.run_test_case('text_visibility.pdf', ['--correct-text-visibility', 1])

    def test_process_form(self):
        self.run_test_case('with_form.pdf', ['--process-form', 1])

    def test_invalid_unicode_issue477(self):
        self.run_test_case('invalid_unicode_issue477.pdf')

    def test_svg_background_with_page_rotation_issue402(self):
        self.run_test_case('svg_background_with_page_rotation_issue402.pdf', ['--bg-format', 'svg'])

    def test_fontfile3_opentype(self):
        self.run_test_case('fontfile3_opentype.pdf', ['-l', 1])
