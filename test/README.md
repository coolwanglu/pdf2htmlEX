### Dependencies

- python2 and packages
  - Python Imaging Library
  - Selenium
  - unittest
- Firefox

### Usage
- Run all tests:
  - `./test.py`
- Run selected test suites:
  - `./test.py test_local_browser`
- Run selected test case:
  - `./test.py test_local_browser.test_basic_text`
  - Or `./test.py test_basic_text`
- Environment variables:
  - Set `P2H_TEST_SAVE_TMP=1` to keep the temporary files in `/tmp/pdf2htmlEX_test`
  - Set `P2H_TEST_GEN=1` to generate new reference files
  - Set `P2H_TEST_REMOTE=1` to test different browsers using Sauce Labs
    - Install `sauceclient` for Python
    - Set correct values for `SAUCE_USERNAME` and `SAUCE_ACCESS_KEY`
    - Setup a HTTP server at `/` on port 8000
    - Enable Sauce Connect
    - See `.travis.yml` as an example

### Add new test cases

- Make sure you have the proper copyrights.
- Using meaningful file names, a description of the file, or issueXXX.pdf.
- Make each test case minimal:
  - One page only, unless the test case is about multiple pages.
  - Grayscale only, unless the test case is about colors.
  - Remove unnecessary elements.
- [Optional] Include the source files that the PDF file is generated from.
- Add the new PDF file to the correct folder in `test/`, and add a new function in the corresponding Python file
- Run `P2H_TEST_GEN=1 test/test.py test_issueXXX` to generate the reference, assuming that the new function is called `test_issueXXX`

