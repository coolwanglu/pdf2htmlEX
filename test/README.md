### Dependencies

- wkhtmltoimage
- python2
- Python Imaging Library

### Usage
- Run all tests:
  - `./test.py`
- Run selected tests:
  - `./test.py test_A test_B ...`
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
  - Set proper parameters for cropping in `wkhtml2image_args`.
- [Optional] Include the source files that the PDF file is generated from.

