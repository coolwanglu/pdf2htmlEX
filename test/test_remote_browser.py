#!/usr/bin/env python

# Run browser tests through Sauce Labs

import unittest
import sys
import os

from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions
from sauceclient import SauceClient
from browser_tests import BrowserTests

# Set your own environment variables
USERNAME = os.environ.get('SAUCE_USERNAME')
ACCESS_KEY = os.environ.get('SAUCE_ACCESS_KEY')

# The base url that remote browser will access
# Usually a HTTP server should be set up in the folder containing the test cases
# Also Sauce Connect should be enabled
BASEURL='http://localhost:8000/'

SAUCE_OPTIONS = {
    'record-video': 'false',
}

# we want to test the latest stable version
# and 'beta' is usually the best estimation
BROWSER_MATRIX = [
    ('win_ie', {
        'platform': 'Windows 8.1',
        'browserName': 'internet explorer',
        'version': '11',
    }),
    ('win_firefox', {
        'platform': 'Windows 8.1',
        'browserName': 'firefox',
        'version': 'beta',
    }),
    ('win_chrome', {
        'platform': 'Windows 8.1',
        'browserName': 'chrome',
        'version': 'beta',
    }),
    ('mac_firefox', {
        'platform': 'OS X 10.9',
        'browserName': 'firefox',
        'version': 'beta',
    }),
    ('mac_chrome', {
        'platform': 'OS X 10.9',
        'browserName': 'chrome',
        'version': '40.0', # beta is not supported
    }),
    ('linux_firefox', {
        'platform': 'Linux',
        'browserName': 'firefox',
        'version': 'beta',
    }),
    ('linux_chrome', {
        'platform': 'Linux',
        'browserName': 'chrome',
        'version': 'beta',
    }),
]

@unittest.skipIf((not (USERNAME and ACCESS_KEY)), 'Sauce Labs is not available')
class test_remote_browser_base(BrowserTests):
    @classmethod
    def setUpClass(cls):
        super(test_remote_browser_base, cls).setUpClass()
        if not cls.GENERATING_MODE:
            cls.sauce = SauceClient(USERNAME, ACCESS_KEY)
            cls.sauce_url = 'http://%s:%s@ondemand.saucelabs.com:80/wd/hub' % (USERNAME, ACCESS_KEY)

            cls.browser = webdriver.Remote(
                desired_capabilities=cls.desired_capabilities,
                command_executor=cls.sauce_url
            )

            cls.browser.implicitly_wait(30)
            # remote screen may not be large enough for the whole page
            cls.browser.set_window_size(cls.BROWSER_WIDTH, cls.BROWSER_HEIGHT)

    @classmethod
    def tearDownClass(cls):
        if not cls.GENERATING_MODE:
            cls.browser.quit()
        super(test_remote_browser_base, cls).tearDownClass()

    def setUp(self):
        super(test_remote_browser_base, self).setUp()
        sys.exc_clear()

    def tearDown(self):
        try:
            passed = (sys.exc_info() == (None, None, None))
            branch = os.environ.get('TRAVIS_BRANCH', 'manual')
            pull_request = os.environ.get('TRAVIS_PULL_REQUEST', 'false')
            self.sauce.jobs.update_job(self.browser.session_id, 
                build_num=os.environ.get('TRAVIS_BUILD_NUMBER', '0'),
                name='pdf2htmlEX',
                passed=passed,
                public='public restricted',
                tags = [pull_request] if pull_request != 'false' else [branch]
            )
        except:
            raise
            pass

    def generate_image(self, html_file, png_file, page_must_load=True):
        self.browser.get(BASEURL + html_file)
        try:
            WebDriverWait(self.browser, 5).until(expected_conditions.presence_of_element_located((By.ID, 'page-container')))
        except:
            if page_must_load:
                raise
        self.browser.save_screenshot(png_file)

test_classnames = []

def generate_classes():
    module = globals()
    for browser_name, browser_caps in BROWSER_MATRIX:
        d = dict(test_remote_browser_base.__dict__)
        caps = SAUCE_OPTIONS.copy()
        caps.update(browser_caps)
        tunnel_identifier = os.environ.get('TRAVIS_JOB_NUMBER')
        if tunnel_identifier:
            caps['tunnel-identifier'] = tunnel_identifier
        d['desired_capabilities'] = caps
        name = "test_remote_%s" % (browser_name, )
        module[name] = type(name, (test_remote_browser_base, unittest.TestCase), d)
        test_classnames.append(name)

generate_classes()
