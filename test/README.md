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
  - `./test.py test_basic_text
- Environment variables:
  - set `P2H_TEST_SAVE_TMP=1` to keep the temporary files
  - set `P2H_TEST_GEN=1` to generate new reference images instead of comparing with old ones

### Guidelines for test cases

- Make sure you have the proper copyrights.
- Using meaningful file names, a description of the file, or issueXXX.pdf.
- Make each test case minimal:
  - One page only, unless the test case is about multiple pages.
  - Grayscale only, unless the test case is about colors.
  - Remove unnecessary elements.
- [Optional] Include the source files that the PDF file is generated from.

