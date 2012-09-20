/*
 * scroll.js
 * render only necessary pages
 *
 * by Hongliang TIAN
 * modifiedy by Lu WANG
 */
$(function() {
  var $pages = $(".p"), 
      $pageWrappers = $(".b"),
      $main  = $("#pdf-main"),
      l = $pages.length;

  function isPageVisible(i, H) {
    var $pw = $($pageWrappers[i]),
        t   = $pw.offset().top,
        b   = t + $pw.height() ;
    return ( ! ( b < 0 || t >= H ) );
  }

  function setVisibilities(from, to, visible) {
    for(var i = from; i <= to; ++i) {
      var $p = $($pages[i]);
      if(visible) $p.show(); else $p.hide();
    }
  }

  // Selectively rendering of pages that are visible or will be visible
  function selectiveRender() {
    var first = 0, last = l - 1,
        H = $main.height();

    // Find the first visible page
    while(first < l && !isPageVisible(first, H))
      first++; 
    // Find the last visible page
    while(last >= 0 && !isPageVisible(last, H))
      last--;
    // Set invisible
    setVisibilities(first > 0 ? first-1 : first, 
                    last < l -1 ? last + 1 : last, true);
    setVisibilities(0, first - 2, false);
    setVisibilities(last + 2, l-1, false);
  }

  // Listen to scrolling events to render proper pages
  var scrolled = false;
  var last_scroll_time = Date.now();
  $("#pdf-main").scroll(function() {
    scrolled = true;
    last_scroll_time = Date.now();
  });

  setInterval(function() {
    // If scrolling pauses 200+ms
    if (scrolled && (Date.now() - last_scroll_time> 100)) {
      scrolled = false;
      // Only render pages that are or will be visible
      selectiveRender();
    }
  }, 100);

  // Trigger the event
  $("#pdf-main").scroll();
});
