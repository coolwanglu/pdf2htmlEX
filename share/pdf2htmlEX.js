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
    default_scale_ratio : 1.0,
    cur_scale_ratio : 1.0,



    /* Constants */
    render_timeout : 200,
    scale_step : 0.9,



    init_before_loading_content : function() {
      /*hide all pages before loading, will reveal only visible ones later */
      this.pre_hide_pages();
    },

    init_after_loading_content : function() {
      this.pages = $(".p");
      this.page_boxes = $(".b");
      this.container = $("#pdf-main");

      if(this.pages.length > 0)
      {
        this.default_scale_ratio = this.cur_scale_ratio 
          = $(this.pages[0]).height() / $(this.page_boxes[0]).height();
      }

      this.selective_render();

      var _ = this;
      this.container.scroll(function(){ _.schedule_render(); });
      this.zoom_fixer();
    },

    init : function() {
      this.init_before_loading_content();

      var _ = this;
      $(document).ready(function(){_.init_after_loading_content();});

      return this;
    },

    pre_hide_pages : function() {
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

    hide_pages : function() {
      for(var i = 0, l = this.page_boxes.length; i < l; ++i)
      {
        $(this.page_boxes[i]).hide();
      }
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

    schedule_render : function() {
      if(this.selective_render_timer)
        clearTimeout(this.selective_render_timer);

      var _ = this;
      this.selective_render_timer = setTimeout(function () {
        _.selective_render();
      }, this.render_timeout);
    },

    zoom_fixer : function () {
      /* 
       * When user try to zoom in/out using ctrl + +/- or mouse wheel
       * handle this and prevent the default behaviours
       *
       * Code credit to PDF.js
       */
      var _ = this;
      // Firefox specific event, so that we can prevent browser from zooming
      $(window).on('DOMMouseScroll', function(e) {
        if (e.ctrlKey) {
          e.preventDefault();
          _.rescale(Math.pow(_.scale_step, e.detail), true);
        }
      });

      $(window).on('keydown', function keydown(e) {
        if (e.ctrlKey || e.metaKey) {
          switch (e.keyCode) {
            case 61: // FF/Mac '='
            case 107: // FF '+' and '='
            case 187: // Chrome '+'
              _.rescale(1.0 / _.scale_step, true);
              break;
            case 173: // FF/Mac '-'
            case 109: // FF '-'
            case 189: // Chrome '-'
              _.rescale(_.scale_step, true);
              break;
            case 48: // '0'
              _.rescale(_.default_scale_ratio, false);
              break;
            default:
              return;
          }
        }
        e.preventDefault();
      });
    },

    rescale : function (ratio, is_relative) {
      console.log('RESCALE');

      if(is_relative)
        ratio *= this.cur_scale_ratio;

      this.cur_scale_ratio = ratio;
      this.hide_pages();

      for(var i = 0, l = this.page_boxes.length; i < l; ++i)
      {
        var p = $(this.pages[i]);
        var pb = $(this.page_boxes[i]);
        p.height(pb.height() * ratio);
        p.width(pb.width() * ratio);
        pb.css('transform', 'scale('+ratio+')');
      }

      this.schedule_render();
    },

    __last_member__ : 'no comma' /*,*/
  }.init();
})();
