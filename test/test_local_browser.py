#!/usr/bin/env python

# Run browsers tests with a local Firefox

import unittest

from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions
from browser_tests import BrowserTests

class test_local_browser(BrowserTests, unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        super(test_local_browser, cls).setUpClass()
        if not cls.GENERATING_MODE:
            cls.browser = webdriver.Firefox()
            cls.browser.maximize_window()
            size = cls.browser.get_window_size()
            assert ((size['width'] >= cls.BROWSER_WIDTH) and (size['height'] >= cls.BROWSER_HEIGHT)), 'Screen is not large enough'
            cls.browser.set_window_size(cls.BROWSER_WIDTH, cls.BROWSER_HEIGHT)

    @classmethod
    def tearDownClass(cls):
        if not cls.GENERATING_MODE:
            cls.browser.quit()
        super(test_local_browser, cls).tearDownClass()

    def generate_image(self, html_file, png_file, page_must_load=True):
        self.browser.get('file://' + html_file)
        try:
            WebDriverWait(self.browser, 5).until(expected_conditions.presence_of_element_located((By.ID, 'page-container')))
        except:
            if page_must_load:
                raise
        self.browser.save_screenshot(png_file)
