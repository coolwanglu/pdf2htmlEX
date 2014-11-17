#!/usr/bin/env python

# Test output files

import unittest
import os

from test import Common

class test_output(Common, unittest.TestCase):
    def run_test_case(self, input_file, expected_output_files, args=[]):
        if self.GENERATING_MODE:
            self.skipTest("Skipping test_output test cases in generating mode")
        args = list(args)
        args.insert(0, os.path.join(self.TEST_DIR, 'test_output', input_file))
        self.assertItemsEqual(self.run_pdf2htmlEX(args)['output_files'], expected_output_files)

    def test_generate_single_html_default_name_single_page_pdf(self):
        self.run_test_case('1-page.pdf', ['1-page.html'])

    def test_generate_single_html_default_name_multiple_page_pdf(self):
        self.run_test_case('2-pages.pdf', ['2-pages.html'])

    def test_generate_single_html_specify_name_single_page_pdf(self):
        self.run_test_case('1-page.pdf', ['foo.html'], ['foo.html'])

    def test_generate_single_html_specify_name_multiple_page_pdf(self):
        self.run_test_case('2-pages.pdf', ['foo.html'], ['foo.html'])

    def test_generate_split_pages_default_name_single_page(self):
        self.run_test_case('1-page.pdf', ['1-page.html', '1-page1.page'], ['--split-pages', 1])

    def test_generate_split_pages_default_name_multiple_pages(self):
        self.run_test_case('3-pages.pdf', ['3-pages.html', '3-pages1.page', '3-pages2.page', '3-pages3.page'], ['--split-pages', 1])

    def test_generate_split_pages_specify_name_single_page(self):
        self.run_test_case('1-page.pdf', ['1-page.html', 'foo1.xyz'], ['--split-pages', 1, '--page-filename', 'foo.xyz'])

    def test_generate_split_pages_specify_name_multiple_pages(self):
        self.run_test_case('3-pages.pdf', ['3-pages.html', 'foo1.xyz', 'foo2.xyz', 'foo3.xyz'], ['--split-pages', 1, '--page-filename', 'foo.xyz'])

    def test_generate_split_pages_specify_name_formatter_multiple_pages(self):
        self.run_test_case('3-pages.pdf', ['3-pages.html', 'fo1o.xyz', 'fo2o.xyz', 'fo3o.xyz'], ['--split-pages', 1, '--page-filename', 'fo%do.xyz'])

    def test_generate_split_pages_specify_name_formatter_with_padded_zeros_multiple_pages(self):
        self.run_test_case('3-pages.pdf', ['3-pages.html', 'fo001o.xyz', 'fo002o.xyz', 'fo003o.xyz'], ['--split-pages', 1, '--page-filename', 'fo%03do.xyz'])

    def test_generate_split_pages_specify_name_only_first_formatter_gets_taken(self):
        self.run_test_case('3-pages.pdf', ['3-pages.html', 'f1o%do.xyz', 'f2o%do.xyz', 'f3o%do.xyz'], ['--split-pages', 1, '--page-filename', 'f%do%do.xyz'])

    def test_generate_split_pages_specify_name_only_percent_d_is_used_percent_s(self):
        self.run_test_case('3-pages.pdf', ['3-pages.html', 'f%soo1.xyz', 'f%soo2.xyz', 'f%soo3.xyz'], ['--split-pages', 1, '--page-filename', 'f%soo.xyz'])

    def test_generate_split_pages_specify_name_only_percent_d_is_used_percent_p(self):
        self.run_test_case('3-pages.pdf', ['3-pages.html', 'f%poo1.xyz', 'f%poo2.xyz', 'f%poo3.xyz'], ['--split-pages', 1, '--page-filename', 'f%poo.xyz'])


    def test_generate_split_pages_specify_name_only_percent_d_is_used_percent_n(self):
        self.run_test_case('3-pages.pdf', ['3-pages.html', 'f%noo1.xyz', 'f%noo2.xyz', 'f%noo3.xyz'], ['--split-pages', 1, '--page-filename', 'f%noo.xyz'])

    def test_generate_split_pages_specify_name_only_percent_d_is_used_percent_percent(self):
        self.run_test_case('3-pages.pdf', ['3-pages.html', 'f%%oo1.xyz', 'f%%oo2.xyz', 'f%%oo3.xyz'], ['--split-pages', 1, '--page-filename', 'f%%oo.xyz'])

    def test_generate_split_pages_specify_name_only_percent_d_is_used_percent_percent_with_actual_placeholder(self):
        self.run_test_case('3-pages.pdf', ['3-pages.html', 'f%%o1o.xyz', 'f%%o2o.xyz', 'f%%o3o.xyz'], ['--split-pages', 1, '--page-filename', 'f%%o%do.xyz'])

    def test_generate_split_pages_specify_name_only_percent_d_is_used_percent_percent_with_actual_placeholder(self):
        self.run_test_case('3-pages.pdf', ['3-pages.html', 'fo1o%%.xyz', 'fo2o%%.xyz', 'fo3o%%.xyz'], ['--split-pages', 1, '--page-filename', 'fo%do%%.xyz'])

    def test_generate_split_pages_specify_name_only_formatter_starts_part_way_through_invalid_formatter(self):
        self.run_test_case('3-pages.pdf', ['3-pages.html', 'f%021oo.xyz', 'f%022oo.xyz', 'f%023oo.xyz'], ['--split-pages', 1, '--page-filename', 'f%02%doo.xyz'])

    def test_generate_split_pages_specify_output_filename_no_formatter_no_extension(self):
        self.run_test_case('1-page.pdf', ['1-page.html', 'foo1'], ['--split-pages', 1, '--page-filename', 'foo'])

    def test_generate_single_html_name_specified_format_characters_percent_d(self):
        self.run_test_case('2-pages.pdf', ['foo%d.html'], ['foo%d.html'])

    def test_generate_single_html_name_specified_format_characters_percent_p(self):
        self.run_test_case('2-pages.pdf', ['foo%p.html'], ['foo%p.html'])

    def test_generate_single_html_name_specified_format_characters_percent_n(self):
        self.run_test_case('2-pages.pdf', ['foo%n.html'], ['foo%n.html'])

    def test_generate_single_html_name_specified_format_characters_percent_percent(self):
        self.run_test_case('2-pages.pdf', ['foo%%.html'], ['foo%%.html'])

