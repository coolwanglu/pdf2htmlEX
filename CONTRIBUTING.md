This is a general guide if you want to report bugs, ask questions,
request features or submitting patches.
Please take a moment to review this document in order to make the 
process easy and effective for everyone involved.

This document is adapted from [necolas/issue-guidelines](https://github.com/necolas/issue-guidelines)

## Table of Contents
- [Channels](#channels)
  - [The Issue Tracker](#the-issue-tracker)
  - [The Mailing List](#the-mailing-list)
  - [Contacting the Author](#contacting-the-author)
- [Guidance](#guidance)
  - [Ask Questions](#ask-questions)
  - [Bug Reports](#bug-reports)
  - [Feature Requests](#feature-requests)
  - [Pull Requests](#pull-requests)

***
## Channels

A few channels are available to reach the developers, please find the most proper one for your purpose.

### The Issue Tracker

The [Issue Tracker](https://github.com/coolwanglu/pdf2htmlEX/issues)
is the best way for 
[bug reports](#bug-reports),
[features requests](#feature-requests) 
and [submitting pull requests](#pull-requests). 

Please respect the following restrictions:
* Do not post personal support requests, (e.g How can I call pdf2htmlEX in Java?). Use the mailing list, or [Stack Overflow](http://stackoverflow.com) instead. 
* Keep the discussion on topic and respect the opinions of others. 
* Posts violating the above restrictions may be removed without any notification.

### The Mailing List

The [Mailing list](pdf2htmlex@googlegroups.com) is set up for discussion and announcements.
You are welcome to [ask any question](#ask-questions) about pdf2htmlEX there. 
However do not report issues or submit patches there, since it's terrible to keep track of them.

### Contacting the author

pdf2htmlEX is mostly written and maintained by 王璐 (Lu Wang).
His email and twitter address of the author can be found in 
[README.md](https://github.com/coolwanglu/pdf2htmlEX/blob/master/README.md). 

Please post only messages that do not fit into the above channels, for example:
- Private messages to the author.
- Sample files with private information. (But please still report the bug via the issue tracker)

Please expect a _long_ delay，since the messages are usually archived and checked on a regular basis.

## Guidance

Here are a few tips for different types of messages.
Lots of your time may be saved if you follow the guidelines.

### Ask questions

If you need any help or have issues using pdf2htmlEX, 
follow the following steps to get it resolved as fast as possible:

First of all, did you realize that your question might have been already answered in one of the following places?

- [pdf2htmlEX Wiki](https://github.com/coolwanglu/pdf2htmlEX/wiki)
- The manpage (run `man pdf2htmlEX`)
- Old posts in the [mailing list](#the-mailing-list) or the [issue tracker](#the-issue-tracker)
- [Google](http://www.google.com/)
 
If you cannot find anything useful there, do not hesitate to post in the [mailing list](#the-mailing-list).
On the other hand, if you think it's something wrong about pdf2htmlEX, please [report a bug](#bug-reports) instead.

### Bug Reports
A bug is a demonstrable problem that is caused by the code in the repository.
A perfect bug report may help the developer to identify the cause and locate the problematic code quickly.
Bugs should always be reported to [the Issue Tracker](#the-issue-tracker).

Before you report any bug:
- Use the latest git version of pdf2htmlEX, since the issue may have been already fixed.
- Search for previous issues (open or closed), to make sure that the issue has not been reported before.

A good bug report shouldn't leave others needing to chase you up for more information.
The developers may be very familiar with the code base of pdf2htmlEX,
but they may not know anything about your environment or what steps you have done, 
unless you have them stated.
Please try to be as detailed as possible in your report.
Good examples include: [#58](https://github.com/coolwanglu/pdf2htmlEX/issues/58), [#183](https://github.com/coolwanglu/pdf2htmlEX/issues/183) and [#226](https://github.com/coolwanglu/pdf2htmlEX/issues/226).

If you are not sure, please try to answer the following questions:

- What's your operating system?
- What's the version of pdf2htmlEX and depended libraries? (You can post the output of `pdf2htmlEX -v`)
- Which browser(s) are you using?
- What steps will reproduce the issue? &mdash; please try to remove unnecessary steps
- What's the result and what did you expect? &mdash; e.g. you can post screenshots
- What error messages did you see?
- Where's the affected PDF file? &mdash; e.g. you may upload the file via Dropbox and post a link here

Especially for issues regarding building pdf2htmlEX:
- Which compiler are you using?
- What's the output of `cmake` and `make`?
- What's the content of `CMakeList.txt`?

### Feature requests

Feature requests are welcome. But take a moment to find out whether your idea
fits with the scope and aims of the project. It's up to *you* to make a strong
case to convince the project's developers of the merits of this feature. Please
provide as much detail and context as possible.

### Pull requests

Good pull requests - patches, improvements, new features - are a fantastic
help. They should remain focused in scope and avoid containing unrelated
commits.

**Please ask first** before embarking on any significant pull request (e.g.
implementing features, refactoring code, porting to a different language),
otherwise you risk spending a lot of time working on something that the
project's developers might not want to merge into the project.

Please read [_Using Pull Requests_](https://help.github.com/articles/using-pull-requests/)
if you are new to pull requests.

**IMPORTANT**: By submitting a patch, you agree to allow the project owner to
license your work under the same license as that used by the project.
