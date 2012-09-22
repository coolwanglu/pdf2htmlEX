/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set shiftwidth=2 tabstop=2 autoindent cindent expandtab: */
/* vim modeline copied from pdf.js */

/*
 * pdf2htmlEX.js
 *
 * handles UI events/actions/effects
 *
 * Copyright 2012 Lu Wang <coolwanglu@gmail.com>
 */

var pdf2htmlEX = (function(){
  return {
    pages : [],
    page_boxes : [],
    container : null,
    selective_render_timer : null,

    init_before_loading_content : function() {
      /*hide all pages before loading, will reveal only visible ones later */
      this.hide_pages();
    },

    init_after_loading_content : function() {
      this.pages = $(".p");
      this.page_boxes = $(".b");
      this.container = $("#pdf-main");

      this.selective_render();

      var _ = this;
      this.container.scroll(function(){
        if(_.selective_render_timer)
          clearTimeout(_.selective_render_timer);
        _.selective_render_timer = setTimeout(function () {
          _.selective_render();
        }, 200);
      });
    },

    init : function() {
      this.init_before_loading_content();

      var _ = this;
      $(document).ready(function(){_.init_after_loading_content();});

      return this;
    },

    hide_pages : function() {
      /* pages might have not been loaded yet, so add a CSS rule */
      var s = '.b{display:none;}';
      var n = document.createElement("style");
      n.type = "text/css";
      if (n.styleSheet) {
        n.styleSheet.cssText = s;
      } else {
        n.appendChild(document.createTextNode(s));
      }
      document.getElementsByTagName("head")[0].appendChild(n);
    },

    selective_render : function () {
      /* hide (positional) invisible pages */
      var l = this.pages.length;
      var ch = this.container.height();
      
      var i;
      for(i = 0; i < l; ++i) {
        var cur_page = $(this.pages[i]);

        if(cur_page.offset().top + cur_page.height() >= 0) break;
        if(i > 0) $(this.page_boxes[i-1]).hide();
      }

      if((i > 0) && (i < l)) $(this.page_boxes[i-1]).show();

      for(; i < l; ++i) {
        $(this.page_boxes[i]).show();

        if($(this.pages[i]).offset().top > ch) break;
      }

      for(++i; i < l; ++i) {
        $(this.page_boxes[i]).hide();
      }
    },

    __last_member__ : 'no comma' /*,*/
  }.init();
})();
