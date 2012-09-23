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
  var EPS = 1e-6;
  var Page = function(page, box, visible) {
    this.p = page;
    this.b = box;
    this.v = visible;
    /*
     * scale ratios
     * 
     * default_r : the first one
     * set_r : last set 
     * cur_r : currently using
     */
    this.default_r = this.set_r = this.cur_r = page.height() / box.height();
  };
  Page.prototype.hide = function(){
    if(this.v) {
      this.v = false;
      this.b.hide();
    }
  };
  Page.prototype.show = function(){
    if(Math.abs(this.set_r - this.cur_r) > EPS) {
      this.cur_r = this.set_r;
      this.b.css('transform', 'scale('+this.cur_r.toFixed(3)+')');
    }
    if(!(this.v)) {
      this.v = true;
      this.b.show();
    }
  };
  Page.prototype.rescale = function(ratio, is_relative) {
    if(ratio == 0) {
      this.set_r = this.default_r;
    } else if (is_relative) {
      this.set_r *= ratio;
    } else {
      this.set_r = ratio;
    }

    /* wait for redraw */
    this.hide();

    this.p.height(this.b.height() * this.set_r);
    this.p.width(this.b.width() * this.set_r);

  };

  return {
    pages : [],
    container : null,
    render_timer : null,

    /* Constants */
    render_timeout : 200,
    scale_step : 0.9,

    init_before_loading_content : function() {
      /*hide all pages before loading, will reveal only visible ones later */
      this.pre_hide_pages();
    },

    init_after_loading_content : function() {
      this.container = $("#pdf-main");

      var new_pages = new Array();
      var pl= $(".p");
      var pbl = $(".b");
      for(var i = 0, l = pl.length; i < l; ++i) {
        new_pages.push(new Page($(pl[i]), $(pbl[i]), false));
      }
      this.pages = new_pages;

      var _ = this;
      this.container.scroll(function(){ _.schedule_render(); });

      //this.zoom_fixer();

      this.render();
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
      var pl = this.pages;
      for(var i = 0, l = pl.length; i < l; ++i)
        pl[i].hide();
    },

    render : function () {
      /* hide (positional) invisible pages */
      var pl = this.pages;
      var l = pl.length;
      var ch = this.container.height();
      
      var i;
      for(i = 0; i < l; ++i) {
        var p = pl[i];

        if(p.p.offset().top + p.p.height() >= 0) break;
        if(i > 0) pl[i-1].hide();
      }

      if((i > 0) && (i < l)) pl[i-1].show();

      for(; i < l; ++i) {
        var p = pl[i];
        p.show();

        if(p.p.offset().top > ch) break;
      }

      for(++i; i < l; ++i) {
        pl[i].hide();
      }
    },

    schedule_render : function() {
      if(this.render_timer)
        clearTimeout(this.render_timer);

      var _ = this;
      this.render_timer = setTimeout(function () {
        _.render();
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
              _.rescale(0, false);
              break;
            default:
              return;
          }
        }
        e.preventDefault();
      });
    },

    rescale : function (ratio, is_relative) {
      var pl = this.pages;
      for(var i = 0, l = pl.length; i < l; ++i) {
        pl[i].rescale(ratio, is_relative);
      }

      this.schedule_render();
    },

    __last_member__ : 'no comma' /*,*/
  }.init();
})();
